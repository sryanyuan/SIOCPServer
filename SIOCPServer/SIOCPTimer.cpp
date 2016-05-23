#include "SIOCPTimer.h"
#include "SIOCPServer.h"
//////////////////////////////////////////////////////////////////////////
static SIOCPTimerPool* s_pTimerPool = NULL;
//////////////////////////////////////////////////////////////////////////
SIOCPTimerPool::SIOCPTimerPool()
{

}

SIOCPTimerPool::~SIOCPTimerPool()
{
	Clear();
}

SIOCPTimerPool* SIOCPTimerPool::GetInstance()
{
	if(NULL == s_pTimerPool)
	{
		s_pTimerPool = new SIOCPTimerPool;
	}
	return s_pTimerPool;
}

void SIOCPTimerPool::DestroyInstance()
{
	if(NULL == s_pTimerPool)
	{
		return;
	}
	delete s_pTimerPool;
	s_pTimerPool = NULL;
}

SIOCPTimer* SIOCPTimerPool::Pop()
{
	SIOCPTimer* pTimer = NULL;

	if(m_xTimerList.empty())
	{
		pTimer = new SIOCPTimer;
	}
	else
	{
		pTimer = m_xTimerList.front();
		m_xTimerList.pop_front();
	}

	memset(pTimer, 0, sizeof(SIOCPTimer));
	return pTimer;
}

void SIOCPTimerPool::Push(SIOCPTimer* _pTimer)
{
	m_xTimerList.push_back(_pTimer);
}

void SIOCPTimerPool::Clear()
{
	SIOCPTimerList::iterator it = m_xTimerList.begin();
	for(it;
		it != m_xTimerList.end();
		++it)
	{
		SIOCPTimer* pTimer = *it;

		delete pTimer;
	}

	m_xTimerList.clear();
}



//////////////////////////////////////////////////////////////////////////
SIOCPTimerControl::SIOCPTimerControl(SIOCPServer* _pServer)
{
	m_pServer = _pServer;
}

SIOCPTimerControl::~SIOCPTimerControl()
{
	Clear();
}


void SIOCPTimerControl::AddTimer(int _nTimerId, int _nInterval, bool _bTriggerOnce)
{
	SIOCPTimer* pTimer = new SIOCPTimer;
	pTimer->nTimerId = _nTimerId;
	pTimer->nInterval = _nInterval;
	pTimer->bTriggerOnce = _bTriggerOnce;
	pTimer->nLastTriggerTime = int(GetTickCount());

	m_xTimerList.push_back(pTimer);
}

void SIOCPTimerControl::RemoveTimer(int _nTimerId)
{
	SIOCPTimerList::iterator it = m_xTimerList.begin();
	for(it;
		it != m_xTimerList.end();
		)
	{
		SIOCPTimer* pTimer = *it;

		if(_nTimerId == pTimer->nTimerId)
		{
			delete pTimer;
			it = m_xTimerList.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void SIOCPTimerControl::Clear()
{
	SIOCPTimerList::iterator it = m_xTimerList.begin();
	for(it;
		it != m_xTimerList.end();
		++it)
	{
		SIOCPTimer* pTimer = *it;

		delete pTimer;
	}

	m_xTimerList.clear();
}

void SIOCPTimerControl::Update()
{
	int nTick = int(GetTickCount());

	SIOCPTimerList::iterator it = m_xTimerList.begin();
	for(it;
		it != m_xTimerList.end();
		)
	{
		SIOCPTimer* pTimer = *it;

		if(nTick - pTimer->nLastTriggerTime > pTimer->nInterval)
		{
			//	trigger
			if(NULL != m_pServer)
			{
				m_pServer->PushEvent(kSIOCPThreadEvent_Timer, (void*)pTimer->nTimerId);
			}

			//	once?
			if(pTimer->bTriggerOnce)
			{
				//	remove
				it = m_xTimerList.erase(it);
				delete pTimer;
			}
			else
			{
				pTimer->nLastTriggerTime = nTick;
				++it;
			}
		}
		else
		{
			++it;
		}
	}
}