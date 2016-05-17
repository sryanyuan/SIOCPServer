#ifndef _INC_SIOCPCONN_
#define _INC_SIOCPCONN_
//////////////////////////////////////////////////////////////////////////
#include <WinSock2.h>
#include "SIOCPBuffer.h"
//////////////////////////////////////////////////////////////////////////
class SIOCPServer;
class SIOCPConn;

class SIOCPOverlapped
{
public:
	SIOCPOverlapped(SIOCPConn* _pConn);

public:
	bool PreRecv(SOCKET _hSock);

public:
	OVERLAPPED m_stOverlapped;
	WSABUF m_stBuf;
	DWORD m_dwNumOfBytesRecvd;
	SIOCPBuffer m_xRecvBuffer;
	SIOCPConn* m_pConn;
	DWORD m_dwFlag;
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
	~SIOCPConn(){}

public:
	unsigned int GetConnIndex();
	void SetConnIndex(unsigned int _uIndex);

	SOCKET GetSocket();
	void SetSocket(SOCKET _hSock);

	SIOCPServer* GetServer();
	void SetServer(SIOCPServer* _pServer);

	SIOCPConnStatusType GetConnStatus();
	void SetConnStatus(SIOCPConnStatusType _eType);

	bool PreRecv();

protected:
	SOCKET m_hSocket;
	unsigned int m_uConnIndex;
	SIOCPOverlapped m_xOverlappedRead;
	SIOCPServer* m_pServer;
	SIOCPConnStatusType m_eConnStatus;
};
//////////////////////////////////////////////////////////////////////////
#endif