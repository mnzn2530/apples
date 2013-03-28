//	������		��	������
//	����ʱ��	��	2011/04/03 11:33:37 
//	��������	��	�̳߳�, �̰߳�ȫ
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

	// �����̳߳ع�ģ
	// nNum: ������ʾ�����̵߳�������������ʾ���ٵ��߳�����
	// ���ص���ǰ���߳�����
	long AdjustSize(long nNum);
	
	// �����̳߳�
	void Call(PFN_ThreadProc pfn, LPVOID pUserData, LPVOID pUserData2);
	void Call(IThreadJob* pJob, LPVOID pUserData, LPVOID pUserData2);

	// �̳߳ع�ģ
	long Size();
	
	// �̳߳����������е��߳�����
	long RunningSize();

	// �̳߳������ڵȴ����е��߳�����
	long WaitingSize();

	// ���������еĹ���������
	long JobSize();

	// �̳߳��Ƿ����߳�������
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