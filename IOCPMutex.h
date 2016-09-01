#ifndef _IOCPMUTEX_H_
#define _IOCPMUTEX_H_

/*
	A Simple IOCP Class
	Author:201724
	Email:number201724@me.com
*/

class IOCPMutex
{
private:
	CRITICAL_SECTION m_csCriticalSection;
public:
	IOCPMutex()
	{
		InitializeCriticalSectionAndSpinCount(&m_csCriticalSection, 4000);
	}
	~IOCPMutex()
	{
		DeleteCriticalSection(&m_csCriticalSection);
	}

	VOID Lock()
	{
		EnterCriticalSection(&m_csCriticalSection);
	}
	VOID UnLock()
	{
		LeaveCriticalSection(&m_csCriticalSection);
	}
};

#endif