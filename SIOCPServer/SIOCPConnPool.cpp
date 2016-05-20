#include "SIOCPConn.h"
//////////////////////////////////////////////////////////////////////////
static SIOCPConnPool* s_pConnPool = NULL;
//////////////////////////////////////////////////////////////////////////
SIOCPConnPool::SIOCPConnPool()
{

}

SIOCPConnPool::~SIOCPConnPool()
{
	Clear();
}

SIOCPConnPool* SIOCPConnPool::GetInstance()
{
	if(NULL == s_pConnPool)
	{
		s_pConnPool = new SIOCPConnPool;
	}
	return s_pConnPool;
}

void SIOCPConnPool::DestroyInstance()
{
	if(NULL != s_pConnPool)
	{
		delete s_pConnPool;
		s_pConnPool = NULL;
	}
}

SIOCPConn* SIOCPConnPool::GetConnection()
{
	SIOCPConn* pConn = NULL;

	if(m_xConnList.empty())
	{
		pConn = new SIOCPConn;
	}
	else
	{
		pConn = m_xConnList.front();
		m_xConnList.pop_front();
	}

	//	initialize the connection
	pConn->Reset();
	
	return pConn;
}

void SIOCPConnPool::FreeConnection(SIOCPConn* _pConn)
{
	m_xConnList.push_back(_pConn);
}

void SIOCPConnPool::Clear()
{
	SIOCPConnList::iterator it = m_xConnList.begin();
	for(it;
		it != m_xConnList.end();
		++it)
	{
		SIOCPConn* pConn = *it;
		delete pConn;
	}
	m_xConnList.clear();
}