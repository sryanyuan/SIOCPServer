#include "SIOCPServer.h"
#include "SIOCPConn.h"
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
			LOGERROR("Get queued completion status failed.");
			continue;
		}

		if(stCompletionKey.GetAction() == kSIOCPCompletionAction_Disconnect ||
			dwNumOfBytesTransferred == 0)
		{
			//	process disconnect
			unsigned short uConnIndex = stCompletionKey.GetIndex();
			//	push event
			pServer->LockDisconnectEventQueue();
			pServer->m_xDisconnectEventQueue.push_back((void*)uConnIndex);
			pServer->UnlockDisconnectEventQueue();
			pServer->EventAwake(kSIOCPThreadEvent_Disconnect);

			continue;
		}
		else if(stCompletionKey.GetAction() == kSIOCPCompletionAction_Destory)
		{
			LOGINFO("Destory completion worker thread");
			break;
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
				unsigned int uIndex = pServer->m_xIndexGenerator.Pop();
				if(0 == uIndex)
				{
					//	failed
					closesocket(sock);
				}
				else
				{
					SIOCPConn* pConn = new SIOCPConn;
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
						//	prepare for read
						pConn->PreRecv();
					}
				}
			}
			pServer->m_xAcceptEventQueue.clear();
			pServer->UnlockAcceptEventQueue();
		}
		else if(dwActiveEvent == kSIOCPCompletionAction_Disconnect)
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
					pServer->Callback_OnDisconnectUser(uIndex);

					closesocket(pConn->GetSocket());
					pConn->SetConnStatus(kSIOCPConnStatus_Disconnected);
					pServer->SetConn(uIndex, NULL);
					pServer->m_xIndexGenerator.Push(pConn->GetConnIndex());
					delete pConn;
					pConn = NULL;
				}
			}
			pServer->m_xDisconnectEventQueue.clear();
			pServer->UnlockDisconnectEventQueue();
		}
	}
}