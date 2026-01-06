#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define FD_SETSIZE			100

#include <iostream>
#include <WinSock2.h>

//#include "ChatPacket.h"

#pragma comment(lib, "ws2_32")



using namespace std;

string PrintAddress(SOCKET InSocket)
{
	SOCKADDR_IN GetSocketAddr;
	memset(&GetSocketAddr, 0, sizeof(GetSocketAddr));
	int GetSocketAddrLength = sizeof(GetSocketAddr);

	getpeername(InSocket, (SOCKADDR*)&GetSocketAddr, &GetSocketAddrLength);

	char Buffer[1024];
	sprintf(Buffer, "%s:%d", inet_ntoa(GetSocketAddr.sin_addr), ntohs(GetSocketAddr.sin_port));

	return Buffer;
}

int main()
{
	WSAData WsaData;
	WSAStartup(MAKEWORD(2, 2), &WsaData);

	SOCKET ListenSocket = socket(PF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN ListenSockAddr;
	memset(&ListenSockAddr, 0, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = AF_INET;
	ListenSockAddr.sin_addr.s_addr = inet_addr("192.168.0.100");
	ListenSockAddr.sin_port = htons(30000);

	bind(ListenSocket, (SOCKADDR*)&ListenSockAddr, sizeof(ListenSockAddr));

	listen(ListenSocket, 5);

	fd_set ReadSocketList;
	FD_ZERO(&ReadSocketList);
	FD_SET(ListenSocket, &ReadSocketList);

	cout << "Start Server" << endl;

	//[1][1][1]  [1][1]   ->   [1][1]
	TIMEVAL Timeout;
	Timeout.tv_sec = 0;
	Timeout.tv_usec = 100;
	while (true)
	{
		fd_set CopyReadSocketList = ReadSocketList;

		int ChangeCount = select(0, &CopyReadSocketList, nullptr, nullptr, &Timeout);
		if (ChangeCount == 0)
		{
			//cout << "Wait" << endl;
			//입력 없을때 서버 작업
			continue;
		}

		//실제 연결처리 및 패킷 처리
		for (int i = 0; i < (int)ReadSocketList.fd_count; ++i)
		{
			SOCKET SelectSocket = ReadSocketList.fd_array[i];
			if (FD_ISSET(SelectSocket, &CopyReadSocketList))
			{
				if (SelectSocket == ListenSocket)
				{
					//연결 요청
					SOCKADDR_IN ClientSockAddr;
					memset(&ClientSockAddr, 0, sizeof(ClientSockAddr));
					int ClientSockAddrLength = sizeof(ClientSockAddr);
					SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientSockAddr, &ClientSockAddrLength);
					FD_SET(ClientSocket, &ReadSocketList);
					cout << "Client connect : " << PrintAddress(ClientSocket) << endl;
				}
				else
				{
					//Header를 다 받을때까지 기다림
					int PacketSize = 0;
					int RecvBytes = recv(SelectSocket, (char*)&PacketSize, sizeof(PacketSize), MSG_WAITALL);

					cout << "RecvBytes : " << RecvBytes << ", PacketSize : " << PacketSize << endl;

					if (RecvBytes == 0)
					{
						cout << "client disconnect : " << PrintAddress(SelectSocket) << endl;
						FD_CLR(SelectSocket, &ReadSocketList);
						closesocket(SelectSocket);
						continue;
					}
					else if (RecvBytes < 0)
					{
						cout << "client error disconnect : " << PrintAddress(SelectSocket) << endl;
						FD_CLR(SelectSocket, &ReadSocketList);
						closesocket(SelectSocket);
						continue;
					}

					PacketSize = ntohl(PacketSize);

					cout << "PacketSize : " << PacketSize << endl;

					//실제 패킷 사이즈만큼 기다림
					char Buffer[4096] = { 0, };
					RecvBytes = recv(SelectSocket, Buffer, PacketSize, MSG_WAITALL);
					if (RecvBytes <= 0)
					{
						cout << "client error disconnect : " << PrintAddress(SelectSocket) << endl;
						FD_CLR(SelectSocket, &ReadSocketList);
						closesocket(SelectSocket);
						continue;
					}

					//Process Packet
					//ChatPacket Data;
					//Data.Parse(Buffer);

					//cout << Data.UserName + " : " << Data.Message << endl;

					PacketSize = htonl(PacketSize);

					for (int j = 0; j < (int)ReadSocketList.fd_count; ++j)
					{
						if (ListenSocket == ReadSocketList.fd_array[j])
						{
							continue;
						}

						//JSON 패킷 크기(header)
						int SendBytes = send(ReadSocketList.fd_array[j], (char*)&PacketSize, sizeof(PacketSize), 0);

						//JSON String
						SendBytes = send(ReadSocketList.fd_array[j], Buffer, RecvBytes, 0);
					}
				}
			}
		}
	}




	closesocket(ListenSocket);


	WSACleanup();


	return 0;

}