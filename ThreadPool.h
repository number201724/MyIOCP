// Filename		: ThreadPool.h
// Author		: Siddharth Barman
// Date			: 18 Sept 2005
/* Description	: Defines CThreadPool class. How to use the Thread Pool. First
				  create a CThreadPool object. The default constructor will 
				  create 10 threads in the pool. To defer creation of the pool
				  pass the pool size and false to the sonstructor. 
				  
				  You can use two approaches while working with the thread 
				  pool. 

				  1. To make use of the thread pool, you will need to first 
				  create a function having the following signature
				  DWORD WINAPI ThreadProc(LPVOID); Check the CreateThread 
				  documentation in MSDN to get details. Add this function to	
				  the pool by calling the Run() method and pass in the function 
				  name and a void* pointer to any object you want. The pool will
				  pick up the function passed into Run() method and execute as 
				  threads in the pool become free. 

				  2. Instead of using a function pointer, you can use an object
				  of a class which derives from IRunObject. Pass a pointer to 
				  this object in the Run() method of the thread pool. As threads
				  become free, the thread pool will call the Run() method of you
				  r class. You will also need to write a body for AutoDelete() f
				  unction. If the return value is true, the thread pool will use
				  'delete' to free the object you pass in. If it returns false,  
				  the thread pool will not do anything else to the object after
				  calling the Run() function.
	
				  It is possible to destroy the pool whenever you want by 
				  calling the Destroy() method. If you want to create a new pool
				  call the Create() method. Make sure you have destoryed the 
				  existing pool before creating a new one.
				  
				  By default, the pool uses _beginthreadex() function to create
				  threads. To have the pool use CreateThread() Windows API, just
				  define USE_WIN32API_THREAD. Note: it is better to use the defa
				  ult _beginthreadex(). 
				  
				  If this code works, it was written by Siddharth Barman, email 
				  siddharth_b@yahoo.com. 
				   _____ _                        _  ______           _  
				  |_   _| |                      | | | ___ \         | | 
				    | | | |__  _ __ ___  __ _  __| | | |_/ /__   ___ | | 
				    | | | '_ \| '__/ _ \/ _` |/ _` | |  __/ _ \ / _ \| | 
				    | | | | | | | |  __/ (_| | (_| | | | | (_) | (_) | | 
				    \_/ |_| |_|_|  \___|\__,_|\__,_| \_|  \___/ \___/|_|  
------------------------------------------------------------------------------*/
#ifndef THREAD_POOL_CLASS
#define THREAD_POOL_CLASS
#pragma warning( disable : 4786) // remove irritating STL warnings

#include <windows.h>
#include <list>
#include <map>
#include "RunObject.h"

#define POOL_SIZE		  20
#define SHUTDOWN_EVT_NAME _T("PoolEventShutdown")
using namespace std;

// info about functions which require servicing will be saved using this struct.
typedef struct tagFunctionData
{
	LPTHREAD_START_ROUTINE lpStartAddress;
	union 
	{
		IRunObject* runObject;
		LPVOID pData;
	};	
} _FunctionData;

// // info about threads in the pool will be saved using this struct.
typedef struct tagThreadData
{
	bool	bFree;
	HANDLE	WaitHandle;
	HANDLE	hThread;
	DWORD	dwThreadId;
} _ThreadData;

// info about all threads belonging to this pool will be stored in this map
typedef map<DWORD, _ThreadData, less<DWORD>, allocator<_ThreadData> > ThreadMap;

// all functions passed in by clients will be initially stored in this list.
typedef list<_FunctionData, allocator<_FunctionData> > FunctionList;

// this decides whether a function is added to the front or back of the list.
enum ThreadPriority
{
	High,
	Low
};

typedef struct UserPoolData
{
	LPVOID pData;
	CThreadPool* pThreadPool;
} _tagUserPoolData;


class CThreadPool
{
private:
	#ifdef USE_WIN32API_THREAD
	static DWORD WINAPI _ThreadProc(LPVOID);
	#else
	static UINT __stdcall _ThreadProc(LPVOID pParam);
	#endif
	
	FunctionList m_functionList;
	ThreadMap m_threads;

	int		m_nPoolSize;
	int		m_nWaitForThreadsToDieMS; // In milli-seconds
	HANDLE	m_hNotifyShutdown; // notifies threads that a new function 
							   // is added
	bool	m_bPoolIsDestroying; // indicates that the pool is being destroyed.
	
	CRITICAL_SECTION m_cs;

	bool	GetThreadProc(DWORD dwThreadId, LPTHREAD_START_ROUTINE&, 
						  LPVOID*, IRunObject**); 
	
	void	FinishNotify(DWORD dwThreadId);
	void	BusyNotify(DWORD dwThreadId);
		
public:
	CThreadPool(int nPoolSize = POOL_SIZE, bool bCreateNow = false, int nWaitTimeForThreadsToComplete = 3000);
	virtual ~CThreadPool();
	bool	Create();	// creates the thread pool
	void	Destroy();	// destroy the thread pool
	HANDLE	GetWaitHandle(DWORD dwThreadId);
	HANDLE	GetShutdownHandle();
	
	int		GetPoolSize();
	void	SetPoolSize(int);
	
	void	Run(LPTHREAD_START_ROUTINE pFunc, LPVOID pData, 
				ThreadPriority priority = Low);

	void	Run(IRunObject*, ThreadPriority priority = Low);

	bool	CheckThreadStop();

	int		GetWorkingThreadCount();
};
//------------------------------------------------------------------------------
#endif
