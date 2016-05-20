#ifndef _INC_SIOCPCONN_
#define _INC_SIOCPCONN_
//////////////////////////////////////////////////////////////////////////
#include <WinSock2.h>
#include <list>
#include "SIOCPBuffer.h"
//////////////////////////////////////////////////////////////////////////
class SIOCPServer;
class SIOCPConn;

#define OVERLAPPED_MODE_NONE	0
#define OVERLAPPED_MODE_READ	1
#define OVERLAPPED_MODE_WRITE	2

class SIOCPOverlapped
{
public:
	SIOCPOverlapped(SIOCPConn* _pConn);
	~SIOCPOverlapped();

public:
	bool PreRecv();
	bool Send(const char* _pData, size_t _ulength);
	bool SendBuffer();
	void Reset();

	void LockSend();
	void UnlockSend();

public:
	OVERLAPPED m_stOverlapped;
	WSABUF m_stBuf;
	DWORD m_dwNumOfBytesRecvd;
	DWORD m_dwNumOfBytesSent;
	SIOCPBuffer m_xDataBuffer;
	SIOCPConn* m_pConn;
	DWORD m_dwRecvFlag;
	DWORD m_dwSendFlag;
	int m_nOverlappedMode;
	bool m_bSending;
	CRITICAL_SECTION m_stSendLock;
	SOCKET m_hSocket;
};

enum SIOCPConnStatusType
{
	kSIOCPConnStatus_None,
	kSIOCPConnStatus_Connecting,
	kSIOCPConnStatus_Connected,
	kSIOCPConnStatus_Disconnected
};

class SIOCPConn
{
public:
	SIOCPConn();
	~SIOCPConn();

public:
	void Reset();

	unsigned int GetConnIndex();
	void SetConnIndex(unsigned int _uIndex);

	SOCKET GetSocket();
	void SetSocket(SOCKET _hSock);

	SIOCPServer* GetServer();
	void SetServer(SIOCPServer* _pServer);

	SIOCPConnStatusType GetConnStatus();
	void SetConnStatus(SIOCPConnStatusType _eType);

	SIOCPOverlapped* GetOverlappedRecv();

	bool PreRecv();
	bool Send(const char* _pData, size_t _uLength);

protected:
	SOCKET m_hSocket;
	unsigned int m_uConnIndex;
	SIOCPOverlapped m_xOverlappedRead;
	SIOCPOverlapped m_xOverlappedSend;
	SIOCPServer* m_pServer;
	SIOCPConnStatusType m_eConnStatus;
};
typedef std::list<SIOCPConn*> SIOCPConnList;

class SIOCPConnPool
{
public:
	~SIOCPConnPool();
protected:
	SIOCPConnPool();

public:
	static SIOCPConnPool* GetInstance();
	static void DestroyInstance();

public:
	SIOCPConn* GetConnection();
	void FreeConnection(SIOCPConn* _pConn);
	void Clear();

private:
	SIOCPConnList m_xConnList;
};
//////////////////////////////////////////////////////////////////////////
#endif