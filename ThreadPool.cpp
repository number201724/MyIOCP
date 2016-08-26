// Filename		: ThreadPool.cpp
// Author		: Siddharth Barman
// Date			: 18 Sept 2005
// Description	: Implementation file for CThreadPool class. 
//------------------------------------------------------------------------------
#include <Windows.h>
#include <process.h>
#include <tchar.h>
#include "ThreadPool.h"


//#ifdef _DEBUG
//#undef THIS_FILE
//static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
//#endif

#define ASSERT(X)
#define TRACE(X)
//------------------------------------------------------------------------------

/* Parameters	: Pointer to a _threadData structure.
   Description	: This is the internal thread function which will run 
				  continuously till the Thread Pool is deleted. Any user thread
				  functions will run from within this function.
*/
#ifdef USE_WIN32API_THREAD
DWORD WINAPI CThreadPool::_ThreadProc(LPVOID pParam)
#else
UINT __stdcall CThreadPool::_ThreadProc(LPVOID pParam)
#endif
{
	//BUGTRAP_THREAD_IN(_ThreadProc)

	DWORD					dwWait;
	CThreadPool*			pool;
  	HANDLE					hThread = GetCurrentThread();
	LPTHREAD_START_ROUTINE  proc;
	LPVOID					data;
	DWORD					dwThreadId = GetCurrentThreadId();
	HANDLE					hWaits[2];
	IRunObject*				runObject;
	bool					bAutoDelete;

	ASSERT(pParam != NULL);
	if(NULL == pParam)
	{
		//BUGTRAP_THREAD_OUT(_ThreadProc)
		return -1;
	}

	pool = static_cast<CThreadPool*>(pParam);
	hWaits[0] = pool->GetWaitHandle(dwThreadId);
	hWaits[1] = pool->GetShutdownHandle();
	
	loop_here:

	dwWait = WaitForMultipleObjects(2, hWaits, FALSE, INFINITE);

	if(dwWait - WAIT_OBJECT_0 == 0)
	{
		if(pool->CheckThreadStop())
		{
			//BUGTRAP_THREAD_OUT(_ThreadProc)
			return 0;
		}
		
		if(pool->GetThreadProc(dwThreadId, proc, &data, &runObject))
		{
			pool->BusyNotify(dwThreadId);
			
			if(proc == NULL)
			{
				runObject->Run();

				runObject->Release();
			}
			else
			{
				// note: the user's data is wrapped inside UserPoolData object
				proc(data);
				
				// Note: data is created by the pool and is deleted by the pool.
				//       The internal user data is not deleted by the pool.
				UserPoolData* pPoolData = static_cast<UserPoolData*>(data);
				delete pPoolData;	
			}
		}
		else
		{
			pool->FinishNotify(dwThreadId); // tell the pool, i am now free
		}

		goto loop_here;
	}
			
	//BUGTRAP_THREAD_OUT(_ThreadProc)
	return 0;
}

//------------------------------------------------------------------------------
/* Parameters	: Pool size, indicates the number of threads that will be 
				  available in the queue.
*******************************************************************************/
CThreadPool::CThreadPool(int nPoolSize, bool bCreateNow, int nWaitTimeForThreadsToComplete)
{
	m_nPoolSize = nPoolSize;
	m_nWaitForThreadsToDieMS = nWaitTimeForThreadsToComplete;
	m_bPoolIsDestroying = false;
	m_hNotifyShutdown = NULL;

	// this is used to protect the shared data like the list and map
	InitializeCriticalSection(&m_cs); 

	if(bCreateNow)
	{
		if(!Create())
		{
			throw 1;
		}
	}
}
//------------------------------------------------------------------------------

/* Description	: Use this method to create the thread pool. The constructor of
				  this class by default will create the pool. Make sure you 
				  do not call this method without first calling the Destroy() 
				  method to release the existing pool.
   Returns		: true if everything went fine else returns false.
  *****************************************************************************/
bool CThreadPool::Create()
{
	HANDLE		hThread;
	DWORD		dwThreadId;
	_ThreadData ThreadData;
	TCHAR		szEvtName[20];
	UINT		uThreadId;	
	
	m_bPoolIsDestroying = false;

	// create the event which will signal the threads to stop
	//m_hNotifyShutdown = CreateEvent(NULL, TRUE, FALSE, SHUTDOWN_EVT_NAME);
	m_hNotifyShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
	//ASSERT(m_hNotifyShutdown != NULL);
	if(!m_hNotifyShutdown)
	{
		return false;
	}

	// create the threads
	for(int nIndex = 0; nIndex < m_nPoolSize; nIndex++)
	{
		_stprintf(szEvtName, _T("Thread%d"), nIndex);
				
		#ifdef USE_WIN32API_THREAD
		hThread = CreateThread(NULL, 0, CThreadPool::_ThreadProc, 
							   this, CREATE_SUSPENDED, &dwThreadId);
		#else
		hThread = (HANDLE)_beginthreadex(NULL, 0, CThreadPool::_ThreadProc, this,  
								 CREATE_SUSPENDED, (UINT*)&uThreadId);
		dwThreadId = uThreadId;
		#endif
		ASSERT(NULL != hThread);
		
		if(hThread)
		{
			// add the entry to the map of threads
			ThreadData.bFree		= true;
			//ThreadData.WaitHandle	= CreateEvent(NULL, TRUE, FALSE, szEvtName);
			ThreadData.WaitHandle	= CreateEvent(NULL, TRUE, FALSE, NULL);
			ThreadData.hThread		= hThread;
			ThreadData.dwThreadId	= dwThreadId;
		
			m_threads.insert(ThreadMap::value_type(dwThreadId, ThreadData));		

			ResumeThread(hThread); 
		
			#ifdef _DEBUG
			TCHAR szMessage[256];
			_stprintf(szMessage, _T("Thread created, handle = %d, id = %d\n"), 
					  hThread, dwThreadId);
			TRACE(szMessage);
			#endif
		}
		else
		{
			return false;
		}
	}

	return true;
}
//------------------------------------------------------------------------------

CThreadPool::~CThreadPool()
{
	Destroy();
}
//------------------------------------------------------------------------------

/* Description	: Use this method to destory the thread pool. The destructor of
				  this class will destory the pool anyway. Make sure you 
				  this method before calling a Create() when an existing pool is 
				  already existing.
   Returns		: void
  *****************************************************************************/
void CThreadPool::Destroy()
{
	// Set the state to 'destroying'. Pooled functions might be checking this 
	// state to see if they need to stop.
	m_bPoolIsDestroying = true;				
	
	// tell all threads to shutdown.
	if(!m_hNotifyShutdown)
	{
		return;
	}
	//ASSERT(NULL != m_hNotifyShutdown);
	SetEvent(m_hNotifyShutdown);

	//Sleep(1000); // give the threads a chance to complete

	if(GetWorkingThreadCount() > 0)
	{
		// There are still threads in processing mode..
		// lets give the thread one last chance to finish based on configured setting
		Sleep(m_nWaitForThreadsToDieMS);
	}
	
	ThreadMap::iterator iter;
	_ThreadData ThreadData;
	
	// walk through the events and threads and close them all
	for(iter = m_threads.begin(); iter != m_threads.end(); iter++)
	{
		ThreadData = (*iter).second;		
		CloseHandle(ThreadData.WaitHandle);
		CloseHandle(ThreadData.hThread);
	}

	// close the shutdown event
	CloseHandle(m_hNotifyShutdown);
	m_hNotifyShutdown = NULL;

	// free any memory not released
	FunctionList::iterator funcIter;
	
	for(funcIter = m_functionList.begin(); funcIter != m_functionList.end(); funcIter++) 
	{
		if(funcIter->pData != NULL) 
		{
			delete funcIter->pData;
			funcIter->pData = NULL;
		}
	}

	// empty all collections
	m_functionList.clear();
	m_threads.clear();
}
//------------------------------------------------------------------------------

int CThreadPool::GetPoolSize()
{
	return m_nPoolSize;
}
//------------------------------------------------------------------------------

/* Parameters	: nSize - number of threads in the pool.   
   ****************************************************************************/
void CThreadPool::SetPoolSize(int nSize)
{
	ASSERT(nSize > 0);
	if(nSize <= 0)
	{
		return;
	}

	m_nPoolSize = nSize;
}

//------------------------------------------------------------------------------
HANDLE CThreadPool::GetShutdownHandle()
{
	return m_hNotifyShutdown;
}
//------------------------------------------------------------------------------

/* Parameters	: hThread - Handle of the thread that is invoking this function.
   Return		: A ThreadProc function pointer. This function pointer will be 
			      executed by the actual calling thread.
				  NULL is returned if no functions list is empty.
																			  */
bool CThreadPool::GetThreadProc(DWORD dwThreadId, LPTHREAD_START_ROUTINE& Proc, 
								LPVOID* Data, IRunObject** runObject)
{
	bool result = false;

	_FunctionData			FunctionData;
	FunctionList::iterator	iter;

	// get the first function info in the function list	
	EnterCriticalSection(&m_cs);
	
	iter = m_functionList.begin();

	if(iter != m_functionList.end())
	{
		FunctionData = (*iter);

		Proc = FunctionData.lpStartAddress;
		
		if(NULL == Proc) // is NULL for function objects
		{		
			*runObject = static_cast<IRunObject*>(FunctionData.pData);
		}
		else		
		{
			// this is a function pointer
			*Data		= FunctionData.pData;
			runObject	= NULL;
		}		

		m_functionList.pop_front(); // remove the function from the list

		result = true;
	}

	LeaveCriticalSection(&m_cs);
	return result;
}
//------------------------------------------------------------------------------

/* Parameters	: hThread - Handle of the thread that is invoking this function.
   Description	: When ever a thread finishes executing the user function, it 
				  should notify the pool that it has finished executing.      
																			  */
void CThreadPool::FinishNotify(DWORD dwThreadId)
{
	ThreadMap::iterator iter;
	
	EnterCriticalSection(&m_cs);
	iter = m_threads.find(dwThreadId);

	if(iter != m_threads.end())	// if search found no elements
	{			
		m_threads[dwThreadId].bFree = true;

#ifdef _DEBUG
		TCHAR szMessage[256];
		_stprintf(szMessage, _T("Thread free, thread id = %d\n"), dwThreadId);
		TRACE(szMessage);
#endif

		if(m_functionList.empty())
		{
			// back to sleep, there is nothing that needs servicing.
			ResetEvent(m_threads[dwThreadId].WaitHandle);
			
		}		
		else
		{
			// there are some more functions that need servicing, lets do that.
			// By not doing anything here we are letting the thread go back and
			// check the function list and pick up a function and execute it.
		}
		
	}
	else
	{	
		//ASSERT(!_T("No matching thread found."));
	}	

	LeaveCriticalSection(&m_cs);
}
//------------------------------------------------------------------------------

void CThreadPool::BusyNotify(DWORD dwThreadId)
{
	ThreadMap::iterator iter;
	
	EnterCriticalSection(&m_cs);

	iter = m_threads.find(dwThreadId);

	if(iter != m_threads.end())	// if search found no elements
	{
		m_threads[dwThreadId].bFree = false;		

#ifdef _DEBUG
		TCHAR szMessage[256];
		_stprintf(szMessage, _T("Thread busy, thread id = %d\n"), dwThreadId);
		TRACE(szMessage);
#endif
	}
	else
	{		
		//ASSERT(!_T("No matching thread found."));
	}	

	LeaveCriticalSection(&m_cs);
}
//------------------------------------------------------------------------------

/* Parameters	: pFunc - function pointer of type ThreadProc
				  pData - An LPVOID pointer
   Decription	: This function is to be called by clients which want to make 
				  use of the thread pool.
  *****************************************************************************/
void CThreadPool::Run(LPTHREAD_START_ROUTINE pFunc, LPVOID pData, 
					  ThreadPriority priority)
{
	_FunctionData funcdata;

	UserPoolData* pPoolData = new UserPoolData();
	
	pPoolData->pData = pData;
	pPoolData->pThreadPool = this;

	funcdata.lpStartAddress = pFunc;
	funcdata.pData			= pPoolData;

	// add it to the list
	EnterCriticalSection(&m_cs);
	if(priority == Low)
	{
		m_functionList.push_back(funcdata);
	}
	else
	{
		m_functionList.push_front(funcdata);
	}
	LeaveCriticalSection(&m_cs);

	// See if any threads are free
	ThreadMap::iterator iter;
	_ThreadData ThreadData;

	EnterCriticalSection(&m_cs);
	for(iter = m_threads.begin(); iter != m_threads.end(); iter++)
	{
		ThreadData = (*iter).second;
		
		if(ThreadData.bFree)
		{
			// here is a free thread, put it to work
			m_threads[ThreadData.dwThreadId].bFree = false;			
			SetEvent(ThreadData.WaitHandle); 
			// this thread will now call GetThreadProc() and pick up the next
			// function in the list.
			break;
		}
	}
	LeaveCriticalSection(&m_cs);
}
//------------------------------------------------------------------------------

/* Parameters	: runObject - Pointer to an instance of class which implements
							  IRunObject interface.
				  priority  - Low or high. Based on this the object will be
							  added to the front or back of the list.
   Decription	: This function is to be called by clients which want to make 
				  use of the thread pool.
  *****************************************************************************/
void CThreadPool::Run(IRunObject* runObject, ThreadPriority priority)
{
	ASSERT(runObject != NULL);
		
	runObject->pThreadPool = this; 

	_FunctionData funcdata;

	funcdata.lpStartAddress = NULL; // NULL indicates a function object is being
									// used instead.
	funcdata.pData			= runObject; // the function object

	// add it to the list
	EnterCriticalSection(&m_cs);
	if(priority == Low)
	{
		m_functionList.push_back(funcdata);
	}
	else
	{
		m_functionList.push_front(funcdata);
	}
	LeaveCriticalSection(&m_cs);

	// See if any threads are free
	ThreadMap::iterator iter;
	_ThreadData ThreadData;

	EnterCriticalSection(&m_cs);
	for(iter = m_threads.begin(); iter != m_threads.end(); iter++)
	{
		ThreadData = (*iter).second;
		
		if(ThreadData.bFree)
		{
			// here is a free thread, put it to work
			m_threads[ThreadData.dwThreadId].bFree = false;			
			SetEvent(ThreadData.WaitHandle); 
			// this thread will now call GetThreadProc() and pick up the next
			// function in the list.
			break;
		}
	}

	LeaveCriticalSection(&m_cs);


}
//------------------------------------------------------------------------------

/* Parameters	: ThreadId - the id of the thread for which the wait handle is 
							 being requested.
   Returns		: NULL if no mathcing thread id is present.
				  The HANDLE which can be used by WaitForXXXObject API.
  *****************************************************************************/
HANDLE CThreadPool::GetWaitHandle(DWORD dwThreadId)
{
	HANDLE hWait;
	ThreadMap::iterator iter;
	
	EnterCriticalSection(&m_cs);
	iter = m_threads.find(dwThreadId);
	
	if(iter == m_threads.end())	// if search found no elements
	{
		LeaveCriticalSection(&m_cs);
		return NULL;
	}
	else
	{		
		hWait = m_threads[dwThreadId].WaitHandle;
		LeaveCriticalSection(&m_cs);
	}	

	return hWait;
}
//------------------------------------------------------------------------------

bool CThreadPool::CheckThreadStop()
{
	// This is function expected to be called by thread functions or IRunObject
	// derived. The expectation is that the code will check this 'property' of
	// the pool and stop it's processing as soon as possible.
	return m_bPoolIsDestroying;
}

//------------------------------------------------------------------------------
 
int CThreadPool::GetWorkingThreadCount()
{
	// Returns true is 
	ThreadMap::iterator iter;
	_ThreadData ThreadData;
	int nCount = 0;

    for(iter = m_threads.begin(); iter != m_threads.end(); iter++) 
	{
		ThreadData = (*iter).second;

        if(!ThreadData.bFree) 
		{
			nCount++;
		}
	}

    return nCount;
}
