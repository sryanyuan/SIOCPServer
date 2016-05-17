// main.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "SIOCPServer.h"

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsa_data;
	WSAStartup(0x0202, &wsa_data);

	SIOCPServer server;
	server.StartServer("127.0.0.1", 2222, 50);
	while(1)
	{
		Sleep(10);
	}
	return 0;
}

