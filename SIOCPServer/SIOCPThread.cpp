#include "SIOCPServer.h"
#include "SIOCPConn.h"
#include "IndexManager.h"
#include "Logger.h"
//////////////////////////////////////////////////////////////////////////
unsigned int SIOCPServer::__acceptThread(void* _pArg)
{
	SIOCPServer* pServer = (SIOCPServer*)_pArg;
	int addrlen = int(sizeof(sockaddr_in));

	//	accept
	while(1)
	{
		SOCKET clientSocket = accept(pServer->m_listenSocket, (sockaddr*)&pServer->m_stSockAddr, &addrlen);
		if(INVALID_SOCKET == clientSocket)
		{
			LOGERROR("Accept invalid socket");
			break;
		}

		//	push event
		pServer->LockAcceptEventQueue();
		pServer->m_xAcceptEventQueue.push_back((void*)clientSocket);
		pServer->UnlockAcceptEventQueue();
		pServer->EventAwake(kSIOCPThreadEvent_Accept);
	}

	LOGINFO("Accept thread end.");
	return 0;
}

unsigned int SIOCPServer::__completionPortWorker(void* _pArg)
{
	SIOCPServer* pServer = (SIOCPServer*)_pArg;
	DWORD dwNumOfBytesTransferred = 0;
	SIOCPCompletionKey stCompletionKey;
	OVERLAPPED* pOverlapped = NULL;

	while(1)
	{
		stCompletionKey.SetData(0);

		if(FALSE == GetQueuedCompletionStatus(pServer->m_hCompletionPort, &dwNumOfBytesTransferred, (DWORD*)&stCompletionKey, &pOverlapped, INFINITE))
		{
			int nErr = WSAGetLastError();
			if(nErr == ERROR_NETNAME_DELETED)
			{
				//	connection closed, continue process and close the socket
				LOGDEBUG("ERROR_NETNAME_DELETED");
			}
			else
			{
				//	do not process
				LOGDEBUG("WSAGetLastError: %d", nErr);
				continue;
			}
		}

		if(stCompletionKey.GetAction() == kSIOCPCompletionAction_Disconnect ||
			dwNumOfBytesTransferred == 0)
		{
			//	process disconnect
			unsigned short uConnIndex = stCompletionKey.GetIndex();
			//	push event
			pServer->PushEvent(kSIOCPThreadEvent_Disconnect, (void*)uConnIndex);
			continue;
		}
		else if(stCompletionKey.GetAction() == kSIOCPCompletionAction_Destory)
		{
			LOGINFO("Destory completion worker thread");
			break;
		}

		SIOCPOverlapped* pIO = (SIOCPOverlapped*)pOverlapped;
		if(NULL == pIO)
		{
			continue;
		}

		if(pIO->m_nOverlappedMode == OVERLAPPED_MODE_READ)
		{
			//	get read status, post to process thread
			unsigned short uConnIndex = stCompletionKey.GetIndex();
			pIO->m_dwNumOfBytesRecvd = dwNumOfBytesTransferred;
			//	push event
			pServer->PushEvent(kSIOCPThreadEvent_Recv, (void*)stCompletionKey.GetData());
		}
		else if(pIO->m_nOverlappedMode == OVERLAPPED_MODE_WRITE)
		{
			//	get write status
			unsigned short uConnIndex = stCompletionKey.GetIndex();
			pIO->m_dwNumOfBytesSent = dwNumOfBytesTransferred;

			//	next send
			pIO->LockSend();

			SIOCPBuffer& refBuffer = pIO->m_xSendBuffer;
			refBuffer.Read(NULL, dwNumOfBytesTransferred);

			//	move data?
			if(refBuffer.GetReadOffset() >= 2 * refBuffer.GetBufferSize() / 3)
			{
				refBuffer.BackwardMove(refBuffer.GetReadOffset());
			}

			//	next read
			if(refBuffer.GetReadableSize() > 0)
			{
				if(!pIO->SendBuffer())
				{
					SIOCPServer::PostDisconnectEvent(pServer, pIO->m_pConn->GetConnIndex());
				}
			}
			else
			{
				pIO->m_bSending = false;
			}

			pIO->UnlockSend();
		}
	}

	LOGINFO("Completion port worker thread end.");
	return 0;
}

unsigned int SIOCPServer::__eventThread(void* _pArg)
{
	SIOCPServer* pServer = (SIOCPServer*)_pArg;

	while(1)
	{
		DWORD dwActiveEvent = WaitForMultipleObjects(kSIOCPThreadEvent_Total,
			pServer->m_hEvents,
			FALSE,
			INFINITE);

		if(dwActiveEvent == kSIOCPThreadEvent_Accept)
		{
			//	accept a new connection
			pServer->LockAcceptEventQueue();

			SIOCPEventQueue::iterator it = pServer->m_xAcceptEventQueue.begin();
			for(it;
				it != pServer->m_xAcceptEventQueue.end();
				++it)
			{
				SOCKET sock = (SOCKET)*it;

				//	get index
				unsigned int uIndex = pServer->m_pIndexGenerator->Pop();
				if(0 == uIndex)
				{
					//	failed
					closesocket(sock);
				}
				else
				{
					//SIOCPConn* pConn = new SIOCPConn;
					SIOCPConn* pConn = SIOCPConnPool::GetInstance()->GetConnection();
					pConn->SetConnIndex(uIndex);
					pConn->SetServer(pServer);
					pConn->SetConnStatus(kSIOCPConnStatus_Connected);
					pConn->SetSocket(sock);
					pServer->SetConn(uIndex, pConn);
					pServer->Callback_OnAcceptUser(uIndex);

					//	bind to completion port
					SIOCPCompletionKey key;
					key.SetIndex((unsigned short)uIndex);
					if(0 == CreateIoCompletionPort((HANDLE)sock, pServer->m_hCompletionPort, key.GetData(), 0))
					{
						LOGERROR("Failed to bind completion port.");
						pServer->Callback_OnDisconnectUser(uIndex);
						pServer->SetConn(uIndex, NULL);
						delete pConn;
						pConn = NULL;
						closesocket(sock);
					}
					else
					{
						LOGDEBUG("Accept new connection %d", pConn->GetConnIndex());
						//	prepare for read
						pConn->PreRecv();
					}
				}
			}
			pServer->m_xAcceptEventQueue.clear();
			pServer->UnlockAcceptEventQueue();
		}
		else if(dwActiveEvent == kSIOCPThreadEvent_Disconnect)
		{
			//	disconnect socket
			pServer->LockDisconnectEventQueue();

			SIOCPEventQueue::iterator it = pServer->m_xDisconnectEventQueue.begin();
			for(it;
				it != pServer->m_xDisconnectEventQueue.end();
				++it)
			{
				unsigned int uIndex = (unsigned int)*it;
				SIOCPConn* pConn = pServer->GetConn(uIndex);

				if(NULL == pConn)
				{
					LOGERROR("Close a unexist socket of index %d", uIndex);
					continue;
				}
				else
				{
					LOGDEBUG("Close a connection %d", pConn->GetConnIndex());
					SIOCPServer::__closeConnection(pConn);
					pConn = NULL;
				}
			}
			pServer->m_xDisconnectEventQueue.clear();
			pServer->UnlockDisconnectEventQueue();
		}
		else if(dwActiveEvent == kSIOCPThreadEvent_Recv)
		{
			//	recv data, check readable
			SIOCPCompletionKey key;
			pServer->LockRecvEventQueue();

			SIOCPEventQueue::iterator it = pServer->m_xRecvEventQueue.begin();
			for(it;
				it != pServer->m_xRecvEventQueue.end();
				++it)
			{
				DWORD dwCompletionKey = (unsigned int)*it;
				key.SetData(dwCompletionKey);
				unsigned int uIndex = key.GetIndex();
				SIOCPConn* pConn = pServer->GetConn(uIndex);

				if(NULL == pConn)
				{
					LOGERROR("Recv data from a unexist socket of index %d", uIndex);
					continue;
				}
				else
				{
					//	parse packet
					LOGDEBUG("Conn %d recv %d bytes", uIndex, pConn->GetOverlappedRecv()->m_dwNumOfBytesRecvd);
					if(__unpackPacket(pConn))
					{
						//	next read
						pConn->PreRecv();
					}
					else
					{
						LOGERROR("Unpack conn %d data failed.close conn.", uIndex);
						SIOCPServer::__closeConnection(pConn);
						pConn = NULL;
					}
				}
			}
			pServer->m_xRecvEventQueue.clear();

			pServer->UnlockRecvEventQueue();
		}
	}
}