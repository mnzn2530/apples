//	创建者		：	黄智寿
//	创建时间	：	2011/04/03 12:32:29 
// -----------------------------------------------------------------------
#include "stdafx.h"
#include "threadpool.h"

CThreadPool::CThreadPool()
: m_nThreadNum(0)
, m_nRunningNum(0)
, m_nJobNum(0)
{}

CThreadPool::~CThreadPool()
{}

BOOL CThreadPool::Open(long nNum /* = Default_Thread_Sizes */)
{
	m_EndEvent.Create(NULL, TRUE, FALSE, NULL);
	m_CompleteEvent.Create(NULL, TRUE, FALSE, NULL);
	m_CallSema.Create(NULL, 0, 0x7FFFFFFF, NULL);
	m_DelSema.Create(NULL, 0, 0x7FFFFFFF, NULL);

	AdjustSize(nNum <= 0 ? Default_Thread_Sizes : nNum);
	return TRUE;
}

void CThreadPool::Close()
{
	_SyncEnd();

	VecThreadItem_It it = m_vecThreadItem.begin();
	for (; it != m_vecThreadItem.end(); ++it)
	{
		if (*it)
		{
			delete *it;
			*it = NULL;
		}
	}
	m_vecThreadItem.clear();
}

long CThreadPool::AdjustSize( long nNum )
{
	if (nNum > 0)
	{
		ThreadItem* pThreadItem = NULL;
		m_csVec.Enter();
		for (int i = 0; i < nNum; ++i)
		{
			pThreadItem = new ThreadItem(this);
			pThreadItem->m_ThreadHandle = CHandle(
				(HANDLE)::_beginthreadex(NULL, 0, _WorkThread, pThreadItem, 0, NULL));

			m_vecThreadItem.push_back(pThreadItem);
		}
		m_csVec.Leave();
	}
	else
	{
		nNum *= -1;
		m_DelSema.Release(nNum > m_nThreadNum ? m_nThreadNum : nNum, NULL);
	}
	return m_nThreadNum;
}

void CThreadPool::Call(PFN_ThreadProc pfn, LPVOID pUserData, LPVOID pUserData2)
{
	// 线程池中没有线程，不添加任务到队列
	if (m_nThreadNum <= 0)
		return;

	m_csQueue.Enter();
	m_JobQueue.push(new JobItem(pfn, pUserData, pUserData2));
	::InterlockedIncrement(&m_nJobNum);
	m_csQueue.Leave();
	m_CallSema.Release(1, NULL);
}

void CThreadPool::Call(IThreadJob* pJob, LPVOID pUserData, LPVOID pUserData2)
{
	Call(_DefaultCallProc, new CallProcParam(pJob, pUserData, pUserData2), NULL);
}

long CThreadPool::Size()
{
	return m_nThreadNum;
}

long CThreadPool::RunningSize()
{
	return m_nRunningNum;
}

long CThreadPool::WaitingSize()
{
	return (long)m_JobQueue.size();
}

long CThreadPool::JobSize()
{
	return m_nJobNum;
}

BOOL CThreadPool::IsRunning()
{
	return m_nRunningNum > 0;
}
//------------------------------------------------------------------------
unsigned int WINAPI CThreadPool::_WorkThread( LPVOID lParam )
{
	ThreadItem* pThreadItem = (ThreadItem*)lParam;
	ATLASSERT(pThreadItem);

	CThreadPool* pThis = pThreadItem->m_pThis;
	ATLASSERT(pThis);

	::InterlockedIncrement(&pThis->m_nThreadNum);

	HANDLE hWaitHandle[3];
	hWaitHandle[0] = pThis->m_CallSema;
	hWaitHandle[1] = pThis->m_DelSema;
	hWaitHandle[2] = pThis->m_EndEvent;

	JobItem* pJob = NULL;
	BOOL bHasJob = TRUE;

	for(;;)
	{
		DWORD dwRet = ::WaitForMultipleObjects(3, hWaitHandle, FALSE, INFINITE);

		// 删除线程
		if (dwRet == WAIT_OBJECT_0 + 1)
			break;

		// 收到退出信号
		if (dwRet == WAIT_OBJECT_0 + 2)
			break;

		// 取作业
		pThis->m_csQueue.Enter();
		bHasJob= pThis->m_JobQueue.empty() ? FALSE : TRUE;
		if (bHasJob)
		{
			pJob = pThis->m_JobQueue.front();
			pThis->m_JobQueue.pop();
			ATLASSERT(pJob);
			::InterlockedDecrement(&pThis->m_nJobNum);
		}
		pThis->m_csQueue.Leave();

		// 收到退出信号并且这时没有作业
		//if (dwRet == WAIT_OBJECT_0 + 2 && bHasJob == FALSE)
		//	break;

		if (bHasJob && pJob)
		{
			::InterlockedIncrement(&pThis->m_nRunningNum);
			pThreadItem->m_IsRunning = TRUE;
			if (pJob->m_pfn)
				pJob->m_pfn(pJob->m_pUserData, pJob->m_pUserData2);

			delete pJob;
			pThreadItem->m_IsRunning = FALSE;
			::InterlockedDecrement(&pThis->m_nRunningNum);
		}
		::Sleep(10);// 释放CPU
	}

	// 清理
	pThis->m_csVec.Enter();
	VecThreadItem_It it = std::find(
		pThis->m_vecThreadItem.begin(), 
		pThis->m_vecThreadItem.end(), 
		pThreadItem);

	if (it != pThis->m_vecThreadItem.end())
		pThis->m_vecThreadItem.erase(it);
	pThis->m_csVec.Leave();

	delete pThreadItem;

	::InterlockedDecrement(&pThis->m_nThreadNum);

	// 所有线程结束
	if (pThis->m_nThreadNum <= 0)
		pThis->m_CompleteEvent.Set();

	return 0;
}

void CThreadPool::_DefaultCallProc(LPVOID lParam, LPVOID lParam2)
{
	CallProcParam* pParam = (CallProcParam*)lParam;
	ATLASSERT(pParam);

	pParam->m_pJob->Run(pParam->m_pUserData, pParam->m_pUserData2);
	delete pParam;
}

void CThreadPool::_SyncEnd()
{
	m_EndEvent.Set();
	::WaitForSingleObject(m_CompleteEvent, INFINITE);

	// 线程池里面的所有线程退出完毕，清理工作项队列
	while(!m_JobQueue.empty())
	{
		JobItem* pJob = m_JobQueue.front();
		if (pJob)
		{
			delete pJob;
			pJob = NULL;
		}
		m_JobQueue.pop();
	}
}