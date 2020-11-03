#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtReceiver.h"
#define windowLen 4
#define seqLen 8


SRReceiver::SRReceiver() :expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRReceiver::~SRReceiver()
{
}

void SRReceiver::receive(const Packet& packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确
	if (checkSum == packet.checksum)
	{
		//若报文还未收到过
		if ((this->expectSequenceNumberRcvd <= packet.seqnum && this->expectSequenceNumberRcvd + windowLen > packet.seqnum) || packet.seqnum < this->expectSequenceNumberRcvd + windowLen - seqLen)
		{
			//若接受报文为期望报文，则将其传递给应用层，并将缓冲区中在他之后的内容传递给应用层
			if (this->expectSequenceNumberRcvd == packet.seqnum)
			{
				pUtils->printPacket("接收方正确收到发送方的报文", packet);
				//取出Message，向上递交给应用层
				Message msg;
				memcpy(msg.data, packet.payload, sizeof(packet.payload));
				pns->delivertoAppLayer(RECEIVER, msg);
				this->expectSequenceNumberRcvd++;
				if (this->expectSequenceNumberRcvd == seqLen)
					this->expectSequenceNumberRcvd = 0;
				//扫描缓冲区
				while (true)
				{
					auto i = this->buffer.begin();
					for (; i < this->buffer.end(); i++)
					{
						if (i->seqnum == this->expectSequenceNumberRcvd)
							break;
					}
					if (i == this->buffer.end())
						break;
					memcpy(msg.data, i->payload, sizeof(i->payload));
					pns->delivertoAppLayer(RECEIVER, msg);
					this->buffer.erase(i);
					this->expectSequenceNumberRcvd++;
					if (this->expectSequenceNumberRcvd == seqLen)
						this->expectSequenceNumberRcvd = 0;
				}
				//输出窗口
				printf("接收方: ");
				int windowBegin = this->expectSequenceNumberRcvd;
				int windowEnd = (windowBegin + windowLen - 1) % seqLen;
				for (int i = 0; i < seqLen; i++)
				{
					if (windowBegin == i)
						printf("{");
					printf(" %d ", i);
					if (windowEnd == i)
						printf("}");
				}
				printf("\n");
			}

			//若接收报文在期望报文之后，且缓冲区中没有相同内容，则将其存入缓冲区
			else
			{
				this->buffer.push_back(packet);
				sort(this->buffer.begin(), this->buffer.end(), [](Packet a, Packet b) {return a.seqnum < b.seqnum; });
				auto i = unique(this->buffer.begin(), this->buffer.end(), [](Packet a, Packet b) {return a.seqnum == b.seqnum; });
				this->buffer.erase(i, this->buffer.end());
				pUtils->printPacket("接收方正确收到发送方失序的报文", packet);
				//输出窗口
				printf("接收方: ");
				int windowBegin = this->expectSequenceNumberRcvd;
				int windowEnd = (windowBegin + windowLen - 1) % seqLen;
				for (int i = 0; i < seqLen; i++)
				{
					if (windowBegin == i)
						printf("{");
					printf(" %d ", i);
					if (windowEnd == i)
						printf("}");
				}
				printf("\n");
			}
		}
		lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
	}
	cout << endl;
}