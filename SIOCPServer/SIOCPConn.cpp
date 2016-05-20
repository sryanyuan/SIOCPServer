#include "SIOCPConn.h"
#include "SIOCPServer.h"
//////////////////////////////////////////////////////////////////////////
SIOCPOverlapped::SIOCPOverlapped(SIOCPConn* _pConn)
{
	memset(&m_stOverlapped, 0, sizeof(m_stOverlapped));
	memset(&m_stBuf, 0, sizeof(m_stBuf));
	m_xDataBuffer.AllocBuffer(32 * 1024);
	InitializeCriticalSection(&m_stSendLock);
	m_pConn = _pConn;
	Reset();
}

SIOCPOverlapped::~SIOCPOverlapped()
{
	DeleteCriticalSection(&m_stSendLock);
}

void SIOCPOverlapped::LockSend()
{
	EnterCriticalSection(&m_stSendLock);
}

void SIOCPOverlapped::UnlockSend()
{
	LeaveCriticalSection(&m_stSendLock);
}

bool SIOCPOverlapped::PreRecv()
{
	if(INVALID_SOCKET == m_hSocket)
	{
		return false;
	}

	m_stBuf.buf = m_xDataBuffer.GetFreeBufferPtr();
	m_stBuf.len = m_xDataBuffer.GetAvailableSize();

	int nRet = WSARecv(m_hSocket, &m_stBuf, 1, &m_dwNumOfBytesRecvd, &m_dwRecvFlag, &m_stOverlapped, NULL);
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

bool SIOCPOverlapped::Send(const char* _pData, size_t _ulength)
{
	bool bRet = true;

	if(INVALID_SOCKET == m_hSocket)
	{
		return false;
	}

	LockSend();
	//	write length
	size_t uFullPacketLength = _ulength + PACKET_HEADER_LENGTH;
	uFullPacketLength = htonl(uFullPacketLength);
	m_xDataBuffer.Write((const char*)&uFullPacketLength, sizeof(size_t));
	m_xDataBuffer.Write(_pData, _ulength);

	if(!m_bSending)
	{
		//	directly send
		bRet = SendBuffer();
	}

	UnlockSend();

	return bRet;
}

bool SIOCPOverlapped::SendBuffer()
{
	bool bRet = true;

	size_t uSendLength = m_xDataBuffer.GetReadableSize();
	if(uSendLength > MAX_SEND_LENGTH_PER_TIME)
	{
		uSendLength = MAX_SEND_LENGTH_PER_TIME;
	}

	m_stBuf.buf = m_xDataBuffer.GetReadableBufferPtr();
	m_stBuf.len = uSendLength;
	int nErr = WSASend(m_hSocket, &m_stBuf, 1, &m_dwNumOfBytesSent, m_dwSendFlag, &m_stOverlapped, NULL);

	if(nErr == SOCKET_ERROR)
	{
		int nErrType = WSAGetLastError();

		if(nErrType == ERROR_IO_PENDING ||
			nErrType == WSAEWOULDBLOCK)
		{
			//	nothing
		}
		else
		{
			bRet = false;
		}
	}

	if(bRet)
	{
		m_bSending = true;
	}

	return bRet;
}

void SIOCPOverlapped::Reset()
{
	memset(&m_stOverlapped, 0, sizeof(m_stOverlapped));
	memset(&m_stBuf, 0, sizeof(m_stBuf));
	m_xDataBuffer.Reset();
	m_xDataBuffer.Reset();
	m_dwNumOfBytesRecvd = 0;
	m_dwNumOfBytesSent = 0;
	m_dwSendFlag = 0;
	m_dwRecvFlag = 0;
	m_nOverlappedMode = OVERLAPPED_MODE_NONE;
	m_bSending = false;
	m_hSocket = INVALID_SOCKET;
}








//////////////////////////////////////////////////////////////////////////
SIOCPConn::SIOCPConn() : m_xOverlappedRead(this),
						m_xOverlappedSend(this)
{
	m_uConnIndex = 0;
	m_hSocket = INVALID_SOCKET;
	m_eConnStatus = kSIOCPConnStatus_None;

	m_xOverlappedRead.m_nOverlappedMode = OVERLAPPED_MODE_READ;
	m_xOverlappedSend.m_nOverlappedMode = OVERLAPPED_MODE_WRITE;
}

SIOCPConn::~SIOCPConn()
{
	
}

void SIOCPConn::Reset()
{
	m_uConnIndex = 0;
	m_hSocket = INVALID_SOCKET;
	m_eConnStatus = kSIOCPConnStatus_None;
	m_xOverlappedRead.Reset();
	m_xOverlappedRead.m_nOverlappedMode = OVERLAPPED_MODE_READ;
	m_xOverlappedSend.Reset();
	m_xOverlappedSend.m_nOverlappedMode = OVERLAPPED_MODE_WRITE;
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
	m_xOverlappedRead.m_hSocket = _hSock;
	m_xOverlappedSend.m_hSocket = _hSock;
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
	bool bRet = m_xOverlappedRead.PreRecv();
	if(!bRet)
	{
		SIOCPServer::PostDisconnectEvent(m_pServer, m_uConnIndex);
	}

	return bRet;
}

SIOCPOverlapped* SIOCPConn::GetOverlappedRecv()
{
	return &m_xOverlappedRead;
}

bool SIOCPConn::Send(const char* _pData, size_t _uLength)
{
	if(INVALID_SOCKET == m_hSocket)
	{
		return false;
	}
	if(kSIOCPConnStatus_Disconnected == m_eConnStatus)
	{
		return false;
	}

	bool bRet = m_xOverlappedSend.Send(_pData, _uLength);
	if(!bRet)
	{
		SIOCPServer::PostDisconnectEvent(m_pServer, m_uConnIndex);
	}

	return bRet;
}