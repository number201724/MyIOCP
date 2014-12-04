#ifndef _IOCPBUFFERQUEUE_H_
#define _IOCPBUFFERQUEUE_H_

/*
	A Simple IOCP Class
	Author:201724
	Email:number201724@me.com
*/

template <class T>
class IOCPQueue
{
private:
	std::list <T> m_vObjectPool;
public:
	T Pop()
	{
		T object = NULL;

		if(m_vObjectPool.empty())
		{
			return object;
		}

		object = m_vObjectPool.front();
		m_vObjectPool.pop_front();

		return object;
	}

	VOID Push(T object)
	{
		m_vObjectPool.push_back(object);
	}

	ULONG Size()
	{
		return m_vObjectPool.size();
	}

	BOOL Empty()
	{
		return m_vObjectPool.empty();
	}

};

#endif