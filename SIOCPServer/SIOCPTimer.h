#ifndef _INC_SIOCPTIMER_
#define _INC_SIOCPTIMER_
//////////////////////////////////////////////////////////////////////////
#include "Def.h"
#include <WinSock2.h>
#include <list>
//////////////////////////////////////////////////////////////////////////
struct SIOCPTimer
{
	int nTimerId;
	int nInterval;
	int nLastTriggerTime;
	bool bTriggerOnce;

	SIOCPTimer()
	{
		nTimerId = 0;
		nInterval = 0;
		nLastTriggerTime = 0;
		bTriggerOnce = false;
	}
};
typedef std::list<SIOCPTimer*> SIOCPTimerList;

class SIOCPTimerPool
{
public:
	~SIOCPTimerPool();
public:
	static SIOCPTimerPool* GetInstance();
	static void DestroyInstance();

protected:
	SIOCPTimerPool();

public:
	void Clear();
	void Push(SIOCPTimer* _pTimer);
	SIOCPTimer* Pop();

private:
	SIOCPTimerList m_xTimerList;
};

class SIOCPServer;

class SIOCPTimerControl
{
public:
	SIOCPTimerControl(SIOCPServer* _pServer);
	~SIOCPTimerControl();

public:
	void AddTimer(int _nTimerId, int _nInterval, bool _bTriggerOnce);
	void RemoveTimer(int _nTimerId);
	void Clear();
	void Update();

private:
	SIOCPTimerList m_xTimerList;
	SIOCPServer* m_pServer;
};
//////////////////////////////////////////////////////////////////////////
#endif