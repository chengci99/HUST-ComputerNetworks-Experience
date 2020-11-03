#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <WS2tcpip.h>
#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <vector>
#include <Windows.h>
#include <bitset>
#include <thread>
#pragma comment(lib, "ws2_32.lib")
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)
#define BUFLEN 2560
#define ROOTADDR "E:\\作业\\3 大三\\计算机通信与网络\\实验\\实验1\\web\\"
/*
#define PORT 80
#define IP_1 127
#define IP_2 0
#define IP_3 0
#define IP_4 1
*/

using namespace std;
//using std::fstream;

bitset<30>down;//记录当前键盘按下状态
bitset<30>pre;//记录前一时刻键盘按下状态
bool running = false;
bool shutting = false;

void check(char c) {//检测某个按键是否按下，按下就改变一下变量
	if (!KEY_DOWN(c))down[c - 'A'] = 0;
	else down[c - 'A'] = 1;
}

struct socket_info
{
	SOCKET socket;//套接字
	string index;//客户请求资源路径
	char recvBUF[BUFLEN];//接收缓冲区
	char sendBUF[BUFLEN];//发送缓冲区
	int openFail;//文件打开错误次数，作为套接字关闭条件
	int sendTime;//文件发送次数，作为套接字关闭条件
	string IPaddr;//客户IP
	socket_info(SOCKET s)//构造函数
	{
		socket = s;
		memset(recvBUF, 0, BUFLEN);
		memset(sendBUF, 0, BUFLEN);
		openFail = 0;
		sendTime = 0;
	}
	int recvBUFlen()//接收缓冲区数据长度
	{
		int i;
		for (i = 0; i < BUFLEN; i++)
			if (recvBUF[i] == '\0')
				break;
		return i;
	}
	int sendBUFlen()//发送缓冲区数据长度
	{
		int i;
		for (i = 0; i < BUFLEN; i++)
			if (sendBUF[i] == '\0')
				break;
		return i;
	}
};

void controlSys()
{
	while (true)
	{
		pre = down;
		check('Q');
		check('B');
		check('S');
		if (pre != down)
		{
			if (down['S' - 'A'])
			{
				running = false;
				printf("Service Stop.\n");
			}
			else if (down['B' - 'A'])
			{
				running = true;
				printf("Service Start.\n");
			}
			else if (down['Q' - 'A'])
			{
				if (!running)
				{
					printf("Program shutting down.\n");
					shutting = true;
					exit(0);
				}
			}
		}
	}
}

int main(int argc, char* argv[])
{
	int PORT, IP_1, IP_2, IP_3, IP_4;
	printf("PORT:");
	scanf("%d", &PORT);
	printf("\nIP:");
	scanf("%d %d %d %d", &IP_1, &IP_2, &IP_3, &IP_4);
	printf("\n");
	thread controlTh(controlSys);
	controlTh.detach();
	//初始化socket
	WORD sockVersion = MAKEWORD(2, 2);
	WSAData wasData;
	if (0 != WSAStartup(sockVersion, &wasData))
	{
		return 0;
	}
	//错误代码或接收字节数
	int nRC;
	//select配套数据，读写socket组
	FD_SET readSet, writeSet;
	vector<socket_info> vSocket;
	timeval timerSelect;
	//创建套接字
	SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//如果为空
	if (slisten == INVALID_SOCKET)
	{
		printf("套接字创建失败");
		return 0;
	}
	//服务器绑定需要的ip和端口
	sockaddr_in sin;             //addr地址
	sin.sin_family = AF_INET;         //ipv4
	sin.sin_port = htons(PORT);         //设置端口
	//sin.sin_addr.S_un.S_addr = INADDR_ANY; //任意地址都可以访问  
	sin.sin_addr.S_un.S_un_b.s_b1 = IP_1;
	sin.sin_addr.S_un.S_un_b.s_b2 = IP_2;
	sin.sin_addr.S_un.S_un_b.s_b3 = IP_3;
	sin.sin_addr.S_un.S_un_b.s_b4 = IP_4;
	//绑定
	if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("绑定地址失败");
		return 0;
	}
	//开始监听
	if (listen(slisten, 5) == SOCKET_ERROR) //可以监听的数量
	{
		printf("监听失败");
		return 0;
	}
	//接听后 通过Tcp传数据
	SOCKET sClient;      //接收套接字
	sockaddr_in remoteAddr; //对方地址
	int nAddrlen = sizeof(remoteAddr);
	printf("Ready to Start\n");
	while (1) //服务器一直服务
	{
		if (shutting)
		{
			getchar();
			return 0;
		}
		if (running)
		{
			printf("*******************working*******************\n");
			//将监听套接字添加到readSet
			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);
			FD_SET(slisten, &readSet);
			for (auto i : vSocket)
			{
				FD_SET(i.socket, &writeSet);
				FD_SET(i.socket, &readSet);
			}
			//select筛选
			timerSelect.tv_sec = 1;
			timerSelect.tv_usec = 0;
			int total = select(0, &readSet, &writeSet, NULL, &timerSelect);
			//若监听套接字已就绪，使用accept接收
			if (FD_ISSET(slisten, &readSet))
			{
				sClient = accept(slisten, (SOCKADDR*)&remoteAddr, &nAddrlen);
				if (sClient == INVALID_SOCKET)
				{
					printf("连接失败\n");
					continue;
				}
				auto tempStruct = socket_info(sClient);
				tempStruct.IPaddr = (string)inet_ntoa(remoteAddr.sin_addr);
				vSocket.push_back(tempStruct);
				printf("收到一个连接:%s \n", inet_ntoa(remoteAddr.sin_addr));
			}
			//遍历所有套接字
			for (auto& i : vSocket)
			{
				//若套接字已准备好接收数据
				if (FD_ISSET(i.socket, &readSet))
				{
					printf("%d  ready to receve from %s:%d\n", i.socket, i.IPaddr.c_str(), PORT);
					memset(i.recvBUF, 0, BUFLEN);
					nRC = recv(i.socket, i.recvBUF, BUFLEN, 0);
					i.recvBUF[nRC] = '\0';
					if (nRC <= 0)
					{
						closesocket(i.socket);
						printf("客户端已关闭\n");
						printf("Socket Closed: %d\n", i.socket);
						for (auto j = vSocket.begin(); j < vSocket.end(); j++)
							if (j->socket == i.socket)
							{
								vSocket.erase(j);
								break;
							}
						continue;
					}
					cout << "****************RECVPACK****************" << endl;
					cout << i.recvBUFlen() << endl;
					cout << i.recvBUF << endl;
					cout << "********************************************" << endl;
					string recvSting;
					recvSting = i.recvBUF;
					int subBegin, subEnd;
					subBegin = recvSting.find("GET") + 5;
					subEnd = recvSting.find("HTTP/1.1") - 5;
					i.index = ROOTADDR + recvSting.substr(subBegin, subEnd);
					//getchar();
				}
				//若套接字已准备好发送数据
				else if (FD_ISSET(i.socket, &writeSet))
				{
					if (i.index.size() == 0)
					{
						closesocket(i.socket);
						vSocket.erase(find_if(vSocket.begin(), vSocket.end(), [i](socket_info a) {return i.socket == a.socket; }));
						continue;
					}
					printf("%d  ready to send\n", i.socket);
					//打开文件，将文件保存入发送缓冲区sendBuf
					cout << i.index << endl;
					ifstream inFileStream;
					inFileStream.open(i.index, ios::in | ios::binary);
					string fileType;
					if (!inFileStream.is_open())
					{
						if (i.openFail == 0)
							printf("文件打开失败\n");
						fileType = "404";
						if (i.index.find(".html") != string::npos || i.index.find(".htm") != string::npos)
							;
						else if (i.index.find(".txt") != string::npos)
							;
						else if (i.index.find(".jpg") != string::npos || i.index.find(".jpeg") != string::npos || i.index.find(".jpe") != string::npos)
							;
						else if (i.index.find(".gif") != string::npos)
							;
						else if (i.index.find(".bmp") != string::npos)
							;
						else if (i.index.find(".ico") != string::npos)
							;
						else if (i.index.find(".mp4") != string::npos)
							;
						else if (i.index.find(".mp3") != string::npos)
							;
						else
							fileType = "400";
						if (fileType == (string)"400")
						{
							fileType = "text/html";
							i.index = ROOTADDR + (string)"400.html";
						}
						if (fileType == (string)"404")
						{
							fileType = "text/html";
							i.index = ROOTADDR + (string)"404.html";
						}
						if (i.openFail++ >= 20)
						{
							closesocket(i.socket);
							printf("文件打开失败次数过多\n");
							printf("Socket Closed: %d\n", i.socket);
							for (auto j = vSocket.begin(); j < vSocket.end(); j++)
								if (j->socket == i.socket)
								{
									vSocket.erase(j);
									break;
								}
							continue;
						}
					}
					else
					{
						inFileStream.seekg(0, inFileStream.end);
						int fileLen = inFileStream.tellg();
						inFileStream.seekg(0, inFileStream.beg);
						if (i.index.find(".html") != string::npos || i.index.find(".htm") != string::npos)
							fileType = "text/html";
						else if (i.index.find(".txt") != string::npos)
							fileType = "text/plain";
						else if (i.index.find(".jpg") != string::npos || i.index.find(".jpeg") != string::npos || i.index.find(".jpe") != string::npos)
							fileType = "image/jpeg";
						else if (i.index.find(".gif") != string::npos)
							fileType = "image/gif";
						else if (i.index.find(".bmp") != string::npos)
							fileType = "image/bmp";
						else if (i.index.find(".ico") != string::npos)
							fileType = "image/x-icon";
						else if (i.index.find(".mp4") != string::npos)
							fileType = "video/mp4";
						else if (i.index.find(".mp3") != string::npos)
							fileType = "audio/mpeg";
						else
							fileType = "400";
						/*
						char errorHead[] = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
						cout << errorHead << endl;
						int sendLen;
						if (((sendLen = send(i.socket, errorHead, strlen(errorHead), 0)) == SOCKET_ERROR))
						{//发送失败
							printf("发送失败！\n");
							break;
						}
						continue;
						*/
						string sendHead;
						cout << "****************SENDPACK****************" << endl;
						sendHead = string("HTTP/1.1 200 OK\r\n");
						sendHead += string("Content-Type: ") + fileType + string("\r\n");
						sendHead += string("Content-Length: ") + to_string(fileLen) + string("\r\n");
						sendHead += string("\r\n");
						cout << sendHead;

						int sendLen;
						if (((sendLen = send(i.socket, sendHead.c_str(), sendHead.length(), 0)) == SOCKET_ERROR))
						{//发送失败
							printf("发送失败！\n");
							break;
						}
						//cout << "pack_size: " << sendLen << endl;
						while (true)
						{
							//缓存清零
							memset(i.sendBUF, 0, BUFLEN);
							inFileStream.read(i.sendBUF, BUFLEN);
							int readLen = inFileStream.gcount();
							if (((sendLen = send(i.socket, i.sendBUF, readLen, 0)) == SOCKET_ERROR))
							{//发送失败
								printf("发送失败！\n");
								break;
							}
							//cout << "pack_size: " << sendLen << endl;
							if (inFileStream.eof())
								break;
						}
						cout << "*************************************" << endl;
						inFileStream.close();
						if (i.sendTime++ > 20)
						{
							closesocket(i.socket);
							printf("重复发送次数过多\n");
							printf("Socket Closed: %d\n", i.socket);
							for (auto j = vSocket.begin(); j < vSocket.end(); j++)
								if (j->socket == i.socket)
								{
									vSocket.erase(j);
									break;
								}
							continue;
						}
					}
				}
			}
		}
		Sleep(100);
	}
	getchar();
	return 0;
}
