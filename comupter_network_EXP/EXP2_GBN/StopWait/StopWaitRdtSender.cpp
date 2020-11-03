#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"
#define windowLen 4
#define seqLen 8


GBNSender::GBNSender() :expectSequenceNumberSend(0), waitingState(false)
{
}


GBNSender::~GBNSender()
{
}



bool GBNSender::getWaitingState() {
	return waitingState;
}




bool GBNSender::send(const Message& message) {
	printf("发送方：sned\n");
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}
	//报文构建
	Packet temPack;
	temPack.acknum = -1; //忽略该字段
	temPack.seqnum = this->expectSequenceNumberSend;
	temPack.checksum = 0;
	memcpy(temPack.payload, message.data, sizeof(message.data));
	temPack.checksum = pUtils->calculateCheckSum(temPack);

	pUtils->printPacket("发送方发送报文", temPack);
	//若等待队列为空，即所有发送数据包均已确认
	if (this->packetWaitingAck.empty())
		pns->startTimer(SENDER, Configuration::TIME_OUT, temPack.seqnum);			//启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, temPack);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方

	this->packetWaitingAck.push_back(temPack);//将当前发送的数据包放入待确认队列尾
	//seq++
	this->expectSequenceNumberSend++;
	if (this->expectSequenceNumberSend == seqLen)
		this->expectSequenceNumberSend = 0;
	//输出窗口
	printf("发送方: ");
	if (!this->packetWaitingAck.empty())
	{
		int windowBegin = packetWaitingAck.front().seqnum;
		int windowEnd = (windowBegin + windowLen - 1) % seqLen;
		for (int i = 0; i < seqLen; i++)
		{
			if (windowBegin == i)
				printf("{");
			if (this->expectSequenceNumberSend == i)
				printf("|");
			printf(" %d ", i);
			if (windowEnd == i)
				printf("}");
		}
		printf("\n");
	}
	else
	{
		for (int i = 0; i < seqLen; i++)
		{
			printf(" %d ", i);
			if (this->expectSequenceNumberSend == i)
				printf("{}");
		}
		printf("\n");
	}
	//若下一个发送序号大于或等于第一个待确认数据包序号与窗口长度之和，则进入等待状态
	if (this->expectSequenceNumberSend >= this->packetWaitingAck.front().seqnum + windowLen || this->expectSequenceNumberSend <= this->packetWaitingAck.front().seqnum + windowLen - seqLen)
		this->waitingState = true;																					//进入等待状态
	cout << endl;
	return true;
}

void GBNSender::receive(const Packet& ackPkt) {
	printf("发送方：receive\n");
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确
	if (checkSum == ackPkt.checksum) {
		if (!this->packetWaitingAck.empty())
		{
			if (!(ackPkt.acknum == seqLen - 1 && this->packetWaitingAck.front().seqnum == 0))
			{
				//如果仍有未确认报文
				if (!this->packetWaitingAck.empty())
				{
					//若确认的报文seqnum还没用完
					if (ackPkt.acknum >= this->packetWaitingAck.front().seqnum)
					{
						//停止计时
						pns->stopTimer(SENDER, this->packetWaitingAck.front().seqnum);		//关闭定时器
						this->waitingState = false;
						//确认ACK之前的报文
						this->packetWaitingAck.erase(this->packetWaitingAck.begin(), this->packetWaitingAck.begin() + (ackPkt.acknum - this->packetWaitingAck.front().seqnum + 1));
						//以第一个等待确认报文序号为参数启动计时
						if (!this->packetWaitingAck.empty())
							pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.front().seqnum);
						pUtils->printPacket("发送方正确收到确认", ackPkt);
						//输出窗口
						if (!this->packetWaitingAck.empty())
						{
							int windowBegin = packetWaitingAck.front().seqnum;
							int windowEnd = (windowBegin + windowLen - 1) % seqLen;
							for (int i = 0; i < seqLen; i++)
							{
								if (windowBegin == i)
									printf("{");
								if (this->expectSequenceNumberSend == i)
									printf("|");
								printf(" %d ", i);
								if (windowEnd == i)
									printf("}");
							}
							printf("\n");
						}
						else
						{
							int windowBegin = this->expectSequenceNumberSend;
							int windowEnd = (windowBegin + windowLen - 1) % seqLen;
							for (int i = 0; i < seqLen; i++)
							{
								if (windowBegin == i)
									printf("{");
								if (this->expectSequenceNumberSend == i)
									printf("|");
								printf(" %d ", i);
								if (windowEnd == i)
									printf("}");
							}
							printf("\n");
						}
					}
					//若确认报文的seqnum已经用完，重新从0开始
					else if (ackPkt.acknum < this->packetWaitingAck.front().seqnum + windowLen - seqLen)
					{
						//停止计时
						pns->stopTimer(SENDER, this->packetWaitingAck.front().seqnum);		//关闭定时器
						this->waitingState = false;
						//确认ACK之前的报文
						this->packetWaitingAck.erase(this->packetWaitingAck.begin(), this->packetWaitingAck.begin() + (ackPkt.acknum + seqLen - this->packetWaitingAck.front().seqnum + 1));
						//以第一个等待确认报文序号为参数启动计时
						if (!this->packetWaitingAck.empty())
							pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.front().seqnum);
						pUtils->printPacket("发送方正确收到确认", ackPkt);
						//输出窗口
						printf("发送方: ");
						if (!this->packetWaitingAck.empty())
						{
							int windowBegin = packetWaitingAck.front().seqnum;
							int windowEnd = (windowBegin + windowLen - 1) % seqLen;
							for (int i = 0; i < seqLen; i++)
							{
								if (windowBegin == i)
									printf("{");
								if (this->expectSequenceNumberSend == i)
									printf("|");
								printf(" %d ", i);
								if (windowEnd == i)
									printf("}");
							}
							printf("\n");
						}
						else
						{
							int windowBegin = this->expectSequenceNumberSend;
							int windowEnd = (windowBegin + windowLen - 1) % seqLen;
							for (int i = 0; i < seqLen; i++)
							{
								if (windowBegin == i)
									printf("{");
								if (this->expectSequenceNumberSend == i)
									printf("|");
								printf(" %d ", i);
								if (windowEnd == i)
									printf("}");
							}
							printf("\n");
						}
					}
				}
			}
		}
	}
	cout << endl;
}

void GBNSender::timeoutHandler(int seqNum) {
	printf("发送方：timeout\n");
	//唯一一个定时器,无需考虑seqNum
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	for (auto i : this->packetWaitingAck)
	{
		pUtils->printPacket("发送方定时器时间到，重发所有等待确认的报文", i);
		pns->sendToNetworkLayer(RECEIVER, i);			//重新发送数据包
	}
	cout << endl;
}
