#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"
#define windowLen 4
#define seqLen 8


TCPSender::TCPSender() :expectSequenceNumberSend(0), waitingState(false)
{
}


TCPSender::~TCPSender()
{
}



bool TCPSender::getWaitingState() {
	return waitingState;
}




bool TCPSender::send(const Message& message) {
	printf("发送方：send\n");
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
	this->expectSequenceNumberSend++;
	if (this->expectSequenceNumberSend == seqLen)
	{
		this->qResendCnt = 0;
		this->expectSequenceNumberSend = 0;
	}
	//若下一个发送序号大于或等于第一个待确认数据包序号与窗口长度之和，则进入等待状态
	if (this->expectSequenceNumberSend >= this->packetWaitingAck.front().seqnum + windowLen || this->expectSequenceNumberSend <= this->packetWaitingAck.front().seqnum + windowLen - seqLen)
		this->waitingState = true;																					//进入等待状态
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
		int windowEnd = (windowBegin + windowLen-1) % seqLen;
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
	cout << endl;
	return true;
}

void TCPSender::receive(const Packet& ackPkt) {
	printf("发送方：receive\n");
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确
	if (checkSum == ackPkt.checksum) {
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		//如果ACK不是回复已经确认的报文，则窗口一定右移
		if (!this->packetWaitingAck.empty())
		{
			if (!(ackPkt.acknum == seqLen - 1 && this->packetWaitingAck.front().seqnum == 0))
			{
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
				//ACK回复已确认报文
				else if (ackPkt.acknum == this->packetWaitingAck.front().seqnum - 1)
				{
					if (this->qResendSeq == ackPkt.acknum)
					{
						this->qResendCnt++;
						//若三次连续相同冗余ACK，需要快速重传
						if (this->qResendCnt == 3)
						{
							this->qResendCnt = 0;
							pns->stopTimer(SENDER, this->packetWaitingAck.front().seqnum);										//首先关闭定时器
							pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.front().seqnum);			//重新启动发送方定时器
							pUtils->printPacket("发送方收到3个相同冗余ACK开始快速重传", this->packetWaitingAck.front());
							pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck.front());			//重新发送数据包
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
					else
					{
						this->qResendSeq = ackPkt.acknum;
						this->qResendCnt = 1;
					}
				}
			}
		}
	}
	cout << endl;
}

void TCPSender::timeoutHandler(int seqNum) {
	printf("发送方：timeout\n");
	//唯一一个定时器,无需考虑seqNum
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	pUtils->printPacket("发送方定时器时间到，重发等待确认的第一个报文", this->packetWaitingAck.front());
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck.front());			//重新发送数据包
	cout << endl;
}
