#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"
#define windowLen 4
#define seqLen 8


SRSender::SRSender() :expectSequenceNumberSend(0), waitingState(false)
{
}


SRSender::~SRSender()
{
}



bool SRSender::getWaitingState() {
	return waitingState;
}




bool SRSender::send(const Message& message) {
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
	this->expectSequenceNumberSend++;
	if (this->expectSequenceNumberSend == seqLen)
		this->expectSequenceNumberSend = 0;
	pns->startTimer(SENDER, Configuration::TIME_OUT, temPack.seqnum);			//启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, temPack);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方

	this->packetWaitingAck.push_back(temPack);//将当前发送的数据包放入待确认队列尾
	printf("发送方: ");
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
	//若下一个发送序号大于或等于第一个待确认数据包序号与窗口长度之和，则进入等待状态
	if (this->expectSequenceNumberSend >= this->packetWaitingAck.front().seqnum + windowLen || this->expectSequenceNumberSend <= this->packetWaitingAck.front().seqnum + windowLen - seqLen)
		this->waitingState = true;																					//进入等待状态
	cout << endl;
	return true;
}

void SRSender::receive(const Packet& ackPkt) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确
	if (checkSum == ackPkt.checksum) {
		//如果ACK不是回复已经确认的报文
		if (!this->packetWaitingAck.empty())
		{
			if (ackPkt.acknum >= this->packetWaitingAck.front().seqnum)
			{
				//再待确认报文队列中寻找该序号
				for (auto i = this->packetWaitingAck.begin(); i < this->packetWaitingAck.end(); i++)
				{
					//若找到该序号对应报文，则将其移除，停止对应计时器
					if (i->seqnum == ackPkt.acknum)
					{
						if (i == this->packetWaitingAck.begin())
							this->waitingState = false;
						pns->stopTimer(SENDER, i->seqnum);		//关闭定时器
						this->packetWaitingAck.erase(i);//将该报文移出等待队列
						break;
					}
				}

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
				/*
				if (!this->packetWaitingAck.empty())
				{
					int windowBegin = packetWaitingAck.front().seqnum;
					int windowEnd = (this->packetWaitingAck.end() - 1)->seqnum;
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
				else
				{
					for (int i = 0; i < seqLen; i++)
					{
						printf(" %d ", i);
						if (this->expectSequenceNumberSend == i)
							printf("{}");
					}
					printf("\n");
				}*/
			}
		}
	}
	cout << endl;
}

void SRSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	for (auto i : this->packetWaitingAck)
	{
		if (i.seqnum == seqNum)
		{
			pUtils->printPacket("发送方定时器时间到，重发超时的报文", i);
			pns->sendToNetworkLayer(RECEIVER, i);			//重新发送数据包
		}
	}
	cout << endl;
}
