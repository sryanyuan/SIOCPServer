// main.cpp : 定义控制台应用程序的入口点。
//

#ifndef _LIB

#include "stdafx.h"
#include "SIOCPServer.h"
#include "Logger.h"
#include <crtdbg.h>

SIOCPServer server;

void onAccept(unsigned int _index)
{
	LOGINFO("accept %d", _index);
}

void onDisconnect(unsigned int _index)
{
	LOGINFO("disconnect %d", _index);
}

void onRecv(unsigned int _index, const char* _data, unsigned int _len)
{
	LOGINFO("recv %d data len %d", _index, _len);

	//	echo
	server.Send(_index, _data, _len);

	//SIOCPServer::PostDisconnectEvent(&server, _index);
}

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsa_data;
	WSAStartup(0x0202, &wsa_data);

	{
		server.StartServer("127.0.0.1", 2222, 50);
		server.SetEventCallback(onAccept, onDisconnect, onRecv);

		getchar();
		server.Shutdown();
		getchar();
	}

	WSACleanup();

#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}

#endif