#include "SIOCPServer.h"
#include "Logger.h"
#include "SIOCPConn.h"
#include <process.h>
//////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "ws2_32.lib")
//////////////////////////////////////////////////////////////////////////
SIOCPServer::SIOCPServer()
{
	m_fnOnAcceptUser = NULL;
	m_fnOnDisconnectUser = NULL;
	m_fnOnRecvFromUser = NULL;

	m_listenSocket = INVALID_SOCKET;
	memset(&m_stSockAddr, 0, sizeof(sockaddr_in));
	memset(m_hEvents, 0, sizeof(m_hEvents));

	m_hAcceptThread = INVALID_HANDLE_VALUE;
	m_hEventThread = INVALID_HANDLE_VALUE;
	m_hCompletionPort = NULL;
	for(int i = 0;  i < sizeof(m_hCompletionPortThreads) / sizeof(m_hCompletionPortThreads[0]); ++i)
	{
		m_hCompletionPortThreads[i] = INVALID_HANDLE_VALUE;
	}

	InitializeCriticalSection(&m_stAcceptEventLock);
	InitializeCriticalSection(&m_stDisconnectEventLock);
	InitializeCriticalSection(&m_stRecvEventLock);
}

SIOCPServer::~SIOCPServer()
{
	DeleteCriticalSection(&m_stAcceptEventLock);
	DeleteCriticalSection(&m_stDisconnectEventLock);
	DeleteCriticalSection(&m_stRecvEventLock);
}


int SIOCPServer::StartServer(const char* _pszHost, unsigned short _nPort, int _nMaxConn)
{
	//	create completion port
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(NULL == m_hCompletionPort)
	{
		LOGERROR("Can't create completion port");
		return -1;
	}

	//	create event thread
	for(int i = 0; i < kSIOCPThreadEvent_Total; ++i)
	{
		m_hEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	m_hEventThread = (HANDLE)_beginthreadex(NULL,
		0,
		&SIOCPServer::__eventThread,
		this,
		0,
		NULL);
	if(INVALID_HANDLE_VALUE == m_hEventThread)
	{
		LOGERROR("Failed to create event thread");
		return -1;
	}

	//	create worker threads
	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);

	for(DWORD i = 0; i < si.dwNumberOfProcessors; ++i)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(NULL,
			0,
			&SIOCPServer::__completionPortWorker,
			this,
			0,
			NULL);
		if(INVALID_HANDLE_VALUE == hThread)
		{
			LOGERROR("Failed to create completion port worker thread.");
			return -1;
		}

		m_hCompletionPortThreads[i] = hThread;
	}

	//	init socket
	m_stSockAddr.sin_family = AF_INET;
	m_stSockAddr.sin_port = htons(_nPort);
	m_stSockAddr.sin_addr.s_addr = inet_addr(_pszHost);

	m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET == m_listenSocket)
	{
		LOGERROR("Can't create socket.");
		return -1;
	}

	//	bind
	if(0 != bind(m_listenSocket, (sockaddr*)&m_stSockAddr, sizeof(sockaddr_in)))
	{
		LOGERROR("Bind failed.");
		return -1;
	}

	//	listen
	if(0 != listen(m_listenSocket, SOMAXCONN))
	{
		LOGERROR("Listen failed.");
		return -1;
	}

	//	init index generator
	m_xIndexGenerator.Init(size_t(_nMaxConn));
	m_pConns = new SIOCPConn*[_nMaxConn + 1];
	memset(m_pConns, 0, sizeof(SIOCPConn*) * (_nMaxConn + 1));

	//	start accept thread
	m_hAcceptThread = (HANDLE)_beginthreadex(NULL,
		0,
		&SIOCPServer::__acceptThread,
		this,
		0,
		NULL);
	if(INVALID_HANDLE_VALUE == m_hAcceptThread)
	{
		LOGERROR("Create accept thread failed.");
		return -1;
	}

	return 0;
}

void SIOCPServer::SetEventCallback(FUNC_ONACCEPT _fnOnAccept, FUNC_ONDISCONNECT _fnOnDisconnect, FUNC_ONRECV _fnOnRecv)
{
	m_fnOnAcceptUser = _fnOnAccept;
	m_fnOnDisconnectUser = _fnOnDisconnect;
	m_fnOnRecvFromUser = _fnOnRecv;
}

void SIOCPServer::Callback_OnAcceptUser(unsigned int _uIndex)
{
	if(NULL == m_fnOnAcceptUser)
	{
		return;
	}
	m_fnOnAcceptUser(_uIndex);
}

void SIOCPServer::Callback_OnDisconnectUser(unsigned int _uIndex)
{
	if(NULL == m_fnOnDisconnectUser)
	{
		return;
	}
	m_fnOnDisconnectUser(_uIndex);
}

void SIOCPServer::Callback_OnRecvFromUser(unsigned int _uIndex, const char* _pData, size_t _uLen)
{
	if(NULL == m_fnOnRecvFromUser)
	{
		return;
	}
	m_fnOnRecvFromUser(_uIndex, _pData, _uLen);
}


void SIOCPServer::LockAcceptEventQueue()
{
	EnterCriticalSection(&m_stAcceptEventLock);
}

void SIOCPServer::UnlockAcceptEventQueue()
{
	LeaveCriticalSection(&m_stAcceptEventLock);
}

void SIOCPServer::LockDisconnectEventQueue()
{
	EnterCriticalSection(&m_stDisconnectEventLock);
}

void SIOCPServer::UnlockDisconnectEventQueue()
{
	LeaveCriticalSection(&m_stDisconnectEventLock);
}

void SIOCPServer::LockRecvEventQueue()
{
	EnterCriticalSection(&m_stRecvEventLock);
}

void SIOCPServer::UnlockRecvEventQueue()
{
	LeaveCriticalSection(&m_stRecvEventLock);
}

void SIOCPServer::EventAwake(SIOCPThreadEventType _eType)
{
	HANDLE hEvent = m_hEvents[_eType];
	SetEvent(hEvent);
}


void SIOCPServer::SetConn(unsigned int _uIndex, SIOCPConn* _pConn)
{
	if(_uIndex == 0 ||
		_uIndex > m_xIndexGenerator.GetMaxIndex())
	{
		LOGERROR("Failed to set conn.Index out of range %d", _uIndex);
		return;
	}

	m_pConns[_uIndex] = _pConn;
}

SIOCPConn* SIOCPServer::GetConn(unsigned int _uIndex)
{
	if(_uIndex == 0 ||
		_uIndex > m_xIndexGenerator.GetMaxIndex())
	{
		LOGERROR("Failed to set conn.Index out of range %d", _uIndex);
		return NULL;
	}

	return m_pConns[_uIndex];
}


void SIOCPServer::PostDisconnectEvent(SIOCPServer* _pServer, unsigned int _uIndex)
{
	//	post disconnect
	SIOCPCompletionKey key;
	key.SetIndex(_uIndex);
	key.SetAction(kSIOCPCompletionAction_Disconnect);

	PostQueuedCompletionStatus(_pServer->m_hCompletionPort, 0, key.GetData(), NULL);
}