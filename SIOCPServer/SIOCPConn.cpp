#include "SIOCPConn.h"
#include "SIOCPServer.h"
//////////////////////////////////////////////////////////////////////////
SIOCPOverlapped::SIOCPOverlapped(SIOCPConn* _pConn)
{
	memset(&m_stOverlapped, 0, sizeof(m_stOverlapped));
	memset(&m_stBuf, 0, sizeof(m_stBuf));
	m_xRecvBuffer.AllocBuffer(256 * 1024);
	m_pConn = _pConn;
	m_dwNumOfBytesRecvd = 0;
	m_dwFlag = 0;
}

bool SIOCPOverlapped::PreRecv(SOCKET _hSock)
{
	m_stBuf.buf = m_xRecvBuffer.GetFreeBufferPtr();
	m_stBuf.len = m_xRecvBuffer.GetAvailableSize();

	int nRet = WSARecv(_hSock, &m_stBuf, 1, &m_dwNumOfBytesRecvd, &m_dwFlag, &m_stOverlapped, NULL);
	if(nRet != SOCKET_ERROR)
	{
		return true;
	}

	int nErr = WSAGetLastError();
	if(nErr == ERROR_IO_PENDING)
	{
		return true;
	}

	return false;
}







//////////////////////////////////////////////////////////////////////////
SIOCPConn::SIOCPConn() : m_xOverlappedRead(this)
{
	m_uConnIndex = 0;
	m_hSocket = INVALID_SOCKET;
	m_eConnStatus = kSIOCPConnStatus_None;
}


unsigned int SIOCPConn::GetConnIndex()
{
	return m_uConnIndex;
}

void SIOCPConn::SetConnIndex(unsigned int _uIndex)
{
	m_uConnIndex = _uIndex;
}

SOCKET SIOCPConn::GetSocket()
{
	return m_hSocket;
}

void SIOCPConn::SetSocket(SOCKET _hSock)
{
	m_hSocket = _hSock;
}

SIOCPServer* SIOCPConn::GetServer()
{
	return m_pServer;
}

void SIOCPConn::SetServer(SIOCPServer* _pServer)
{
	m_pServer = _pServer;
}

SIOCPConnStatusType SIOCPConn::GetConnStatus()
{
	return m_eConnStatus;
}

void SIOCPConn::SetConnStatus(SIOCPConnStatusType _eType)
{
	m_eConnStatus = _eType;
}

bool SIOCPConn::PreRecv()
{
	bool bRet = m_xOverlappedRead.PreRecv(m_hSocket);
	if(!bRet)
	{
		SIOCPServer::PostDisconnectEvent(m_pServer, m_uConnIndex);
	}

	return bRet;
}