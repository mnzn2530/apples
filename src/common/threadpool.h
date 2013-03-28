//	创建者		：	黄智寿
//	创建时间	：	2011/04/03 11:33:37 
//	功能描述	：	线程池, 线程安全
// -----------------------------------------------------------------------
#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <vector>
#include <queue>

#define Default_Thread_Sizes 4

struct IThreadJob
{
	virtual void Run(LPVOID lParam, LPVOID lParam2) = 0;
};

class CThreadPool
{
public:
	typedef void (CALLBACK *PFN_ThreadProc)(LPVOID lParam, LPVOID lParam2);

public:
	CThreadPool();
	~CThreadPool();

	BOOL Open(long nNum = Default_Thread_Sizes);
	void Close();

	// 调整线程池规模
	// nNum: 正数表示增加线程的数量，负数表示减少的线程数量
	// 返回调整前的线程数量
	long AdjustSize(long nNum);
	
	// 调用线程池
	void Call(PFN_ThreadProc pfn, LPVOID pUserData, LPVOID pUserData2);
	void Call(IThreadJob* pJob, LPVOID pUserData, LPVOID pUserData2);

	// 线程池规模
	long Size();
	
	// 线程池中正在运行的线程数量
	long RunningSize();

	// 线程池中正在等待运行的线程数量
	long WaitingSize();

	// 工作队列中的工作项数量
	long JobSize();

	// 线程池是否有线程在运行
	BOOL IsRunning();

private:
	static unsigned int WINAPI _WorkThread(LPVOID lParam);
	static void CALLBACK _DefaultCallProc(LPVOID lParam, LPVOID lParam2);
	void _SyncEnd();

private:
	struct CallProcParam
	{
		IThreadJob* m_pJob;
		LPVOID m_pUserData;
		LPVOID m_pUserData2;
		
		CallProcParam(IThreadJob* pJob, LPVOID pUserData, LPVOID pUserData2)
			: m_pJob(pJob)
			, m_pUserData(pUserData)
			, m_pUserData2(pUserData2)
		{}
	};

	struct JobItem
	{
		PFN_ThreadProc m_pfn;
		LPVOID m_pUserData;
		LPVOID m_pUserData2;

		JobItem(PFN_ThreadProc pfn, LPVOID pUserData, LPVOID pUserData2)
			: m_pfn(pfn)
			, m_pUserData(pUserData)
			, m_pUserData2(pUserData2)
		{}
	};
	typedef std::queue<JobItem*> QueueJobItem;

	struct ThreadItem
	{
		CThreadPool* m_pThis;
		CHandle m_ThreadHandle;
		BOOL m_IsRunning;

		ThreadItem(CThreadPool* pThis)
			: m_pThis(pThis)
			, m_ThreadHandle(NULL)
		{}
	};
	typedef std::vector<ThreadItem*> VecThreadItem;
	typedef VecThreadItem::iterator VecThreadItem_It;

private:
	volatile long m_nThreadNum;
	volatile long m_nRunningNum;
	volatile long m_nJobNum;
	QueueJobItem m_JobQueue;
	VecThreadItem m_vecThreadItem;

	CCriticalSection m_csVec;
	CCriticalSection m_csQueue;

	CEvent m_EndEvent;
	CEvent m_CompleteEvent;
	CSemaphore m_CallSema;
	CSemaphore m_DelSema;
};

#endif