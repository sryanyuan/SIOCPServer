#ifndef _INC_SIOCPSERVER_
#define _INC_SIOCPSERVER_
//////////////////////////////////////////////////////////////////////////
#include "Def.h"
#include <WinSock2.h>
#include <list>
//////////////////////////////////////////////////////////////////////////
enum SIOCPCompletionActionType
{
	kSIOCPCompletionAction_Default,
	kSIOCPCompletionAction_Disconnect,
	kSIOCPCompletionAction_Destroy,
};

struct SIOCPCompletionKey
{
	unsigned int uData;

	SIOCPCompletionKey()
	{
		uData = 0;
	}

	unsigned int GetData()
	{
		return uData;
	}
	void SetData(unsigned int _uData)
	{
		uData = _uData;
	}

	unsigned short GetIndex()
	{
		return uData & 0x0000ffff;
	}
	void SetIndex(unsigned short _uIndex)
	{
		uData |= _uIndex;
	}

	unsigned short GetAction()
	{
		return (uData & 0xff000000) >> 24;
	}
	void SetAction(unsigned char _uAction)
	{
		uData |= (_uAction << 24);
	}
};

enum SIOCPThreadEventType
{
	kSIOCPThreadEvent_Accept,
	kSIOCPThreadEvent_Disconnect,
	kSIOCPThreadEvent_Recv,
	KSIOCPThreadEvent_Send,
	kSIOCPThreadEvent_Destroy,
	kSIOCPThreadEvent_Total,
};

struct SIOCPAcceptEvent
{
	SOCKET hSock;
};
typedef std::list<void*> SIOCPEventQueue;

class SIOCPConn;
class IndexManager;

class SIOCPServer
{
	friend class SIOCPOverlapped;

public:
	SIOCPServer();
	virtual ~SIOCPServer();

public:
	int Create();
	int StartServer(const char* _pszHost, unsigned short _nPort, int _nMaxConn);
	void Shutdown();
	void SetEventCallback(FUNC_ONACCEPT _fnOnAccept, FUNC_ONDISCONNECT _fnOnDisconnect, FUNC_ONRECV _fnOnRecv);
	bool Send(unsigned int _uIndex, const char* _pData, size_t _uLength);

protected:
	void Callback_OnAcceptUser(unsigned int _uIndex);
	void Callback_OnDisconnectUser(unsigned int _uIndex);
	void Callback_OnRecvFromUser(unsigned int _uIndex, const char* _pData, size_t _uLen);

	void LockAcceptEventQueue();
	void UnlockAcceptEventQueue();

	void LockDisconnectEventQueue();
	void UnlockDisconnectEventQueue();

	void LockRecvEventQueue();
	void UnlockRecvEventQueue();

	void LockSendEventQueue();
	void UnlockSendEventQueue();

	void PushEvent(SIOCPThreadEventType _eType, void* _pEvt);
	void EventAwake(SIOCPThreadEventType _eType);

	void SetConn(unsigned int _uIndex, SIOCPConn* _pConn);
	SIOCPConn* GetConn(unsigned int _uIndex);

public:
	static void PostDisconnectEvent(SIOCPServer* _pServer, unsigned int _uIndex);
	static void Free();

protected:
	static bool __unpackPacket(SIOCPConn* _pConn);
	static void __closeConnection(SIOCPConn* _pConn);

protected:
	static unsigned int __stdcall __acceptThread(void* _pArg);
	static unsigned int __stdcall __completionPortWorker(void* _pArg);
	static unsigned int __stdcall __eventThread(void* _pArg);

protected:
	//	event thread
	HANDLE m_hEvents[kSIOCPThreadEvent_Total];

	FUNC_ONACCEPT m_fnOnAcceptUser;
	FUNC_ONDISCONNECT m_fnOnDisconnectUser;
	FUNC_ONRECV m_fnOnRecvFromUser;

	SOCKET m_listenSocket;
	sockaddr_in m_stSockAddr;

	HANDLE m_hAcceptThread;
	HANDLE m_hEventThread;
	HANDLE m_hCompletionPort;
	DWORD m_dwCompletionWorkerThreadCount;
	HANDLE m_hCompletionPortThreads[MAX_WORKER_THREAD_COUNT];

	IndexManager* m_pIndexGenerator;

	//	connections
	SIOCPConn** m_pConns;

	//	event queues
	SIOCPEventQueue m_xAcceptEventQueue;
	CRITICAL_SECTION m_stAcceptEventLock;

	SIOCPEventQueue m_xDisconnectEventQueue;
	CRITICAL_SECTION m_stDisconnectEventLock;

	SIOCPEventQueue m_xRecvEventQueue;
	CRITICAL_SECTION m_stRecvEventLock;

	SIOCPEventQueue m_xSendEventQueue;
	CRITICAL_SECTION m_stSendEventLock;

	bool m_bAcceptConnection;
};
//////////////////////////////////////////////////////////////////////////
#endif