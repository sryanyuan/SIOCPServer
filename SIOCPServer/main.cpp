// main.cpp : 定义控制台应用程序的入口点。
//

#ifndef _LIB

#ifdef _DEBUG
#define DEBUG_CLIENTBLOCK  new( _CLIENT_BLOCK, __FILE__, __LINE__)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new DEBUG_CLIENTBLOCK
#endif

#include "stdafx.h"
#include "SIOCPServer.h"
#include "Logger.h"

SIOCPServer* pServer;

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
	pServer->Send(_index, _data, _len);

	//SIOCPServer::PostDisconnectEvent(&server, _index);
}

void onTimer(int _nTimerId)
{
	LOGINFO("timer %d active, tick %d", _nTimerId, GetTickCount());
}

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsa_data;
	WSAStartup(0x0202, &wsa_data);

	if(1)
	{
		pServer = new SIOCPServer;
		pServer->SetEventCallback(onAccept, onDisconnect, onRecv, onTimer);
		pServer->AddTimer(1, 1000, false);
		pServer->StartServer("127.0.0.1", 2222, 50);

		getchar();
		pServer->Shutdown();
		getchar();

		delete pServer;
		pServer = NULL;

		SIOCPServer::Free();
	}

	WSACleanup();

#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}

#endif