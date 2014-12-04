// Filename		: ThreadPool.cs
// Author		: number201724
// Date			: 2014-12-1
// Description	: Implementation file for ExThreadPool class. 
//------------------------------------------------------------------------------
//				   _____ _                        _  ______           _  
//				  |_   _| |                      | | | ___ \         | | 
//				    | | | |__  _ __ ___  __ _  __| | | |_/ /__   ___ | | 
//				    | | | '_ \| '__/ _ \/ _` |/ _` | |  __/ _ \ / _ \| | 
//				    | | | | | | | |  __/ (_| | (_| | | | | (_) | (_) | | 
//				    \_/ |_| |_|_|  \___|\__,_|\__,_| \_|  \___/ \___/|_|  
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace NewCommon.ThreadEx
{
	public class ThreadPoolEx
	{
		/// <summary>
		/// abstract class thread of execution
		/// </summary>
		public interface IRunObject
		{
			void Run();
		}

		//info about functions which require servicing will be saved using this struct.
		private class FunctionData
		{
			public IRunObject RunObject = null;
			public ParameterizedThreadStart ThreadFunc;
			public object param;
		}
		//this decides whether a function is added to the front or back of the list.
		public enum ThreadPriority
		{
			Low,
			High
		}
		/// <summary>
		/// each thread management data
		/// </summary>
		private class ThreadData
		{
			public bool bFree;
			public ManualResetEvent WaitHandle;
			public Thread hThread;
			public int dwThreadId;
		}

		private int m_nPoolSize;

		private bool m_bPoolIsDestroying;		// notifies threads that a new function 

		private bool m_bStarted;
		private EventWaitHandle m_hNotifyShutdown;

		private LinkedList<FunctionData> m_functionList;
		private List<ThreadData> m_threads;

		/// <summary>
		/// construct functions
		/// </summary>
		/// <param name="nPoolSize">set the number of the current thread pool thread default:20</param>
		/// <param name="bCreateNow">whether to create a thread immediately</param>
		public ThreadPoolEx(int nPoolSize = 20, bool bCreateNow = false)
		{
			this.m_functionList = new LinkedList<FunctionData>();
			this.m_threads = new List<ThreadData>();
			this.m_hNotifyShutdown = new EventWaitHandle(false, EventResetMode.ManualReset);

			this.m_nPoolSize = nPoolSize;
			this.m_bPoolIsDestroying = false;
			this.m_bStarted = false;

			if (bCreateNow)
			{
				Create();
			}
		}

		/// <summary>
		/// get the current number of threads in the thread pool
		/// </summary>
		/// <returns></returns>
		public int GetPoolSize()
		{
			return this.m_nPoolSize;
		}

		/// <summary>
		/// set the number of the current thread pool thread
		/// </summary>
		/// <param name="nPoolSize"></param>
		public void SetPoolSize(int nPoolSize)
		{
			if (nPoolSize <= 0)
				return;
			this.m_nPoolSize = nPoolSize;
		}
		/// <summary>
		/// notification thread pool, state of the current thread is finish
		/// </summary>
		/// <param name="pThreadData"></param>
		private void FinishNotify(ThreadData pThreadData)
		{
			lock (this)
			{
				pThreadData.bFree = true;
				pThreadData.WaitHandle.Reset();
			}
		}
		/// <summary>
		/// notification thread pool, state of the current thread is busy
		/// </summary>
		/// <param name="pThreadData"></param>
		private void BusyNotify(ThreadData pThreadData)
		{
			lock (this)
			{
				pThreadData.bFree = false;
			}
		}
		/// <summary>
		/// post an object to the thread pool, and start execution thread pool
		/// </summary>
		/// <param name="func">function thread</param>
		/// <param name="param">thread function parameters</param>
		/// <param name="priority">Low or high. Based on this the object will be added to the front or back of the list.</param>
		public void Run(ParameterizedThreadStart func, object param = null, ThreadPriority priority = ThreadPriority.Low)
		{
			FunctionData functions = new FunctionData();
			functions.RunObject = null;
			functions.ThreadFunc = func;
			functions.param = param;

			lock (this)
			{
				if (priority == ThreadPriority.Low)
				{
					m_functionList.AddLast(functions);
				}
				else
				{
					m_functionList.AddFirst(functions);
				}

				foreach (ThreadData pThreadData in m_threads)
				{
					if (pThreadData.bFree == true)
					{
						pThreadData.bFree = false;
						pThreadData.WaitHandle.Set();
						break;
					}
				}
			}
		}
		/// <summary>
		/// post an object to the thread pool, and start execution thread pool
		/// </summary>
		/// <param name="runObject">Pointer to an instance of class which implements IRunObject interface.</param>
		/// <param name="priority">Low or high. Based on this the object will be added to the front or back of the list.</param>
		public void Run(IRunObject runObject, ThreadPriority priority = ThreadPriority.Low)
		{
			FunctionData functions = new FunctionData();
			functions.RunObject = runObject;
			functions.ThreadFunc = null;
			functions.param = null;

			lock (this)
			{
				if (priority == ThreadPriority.Low)
				{
					m_functionList.AddLast(functions);
				}
				else
				{
					m_functionList.AddFirst(functions);
				}

				foreach (ThreadData pThreadData in m_threads)
				{
					if (pThreadData.bFree == true)
					{
						pThreadData.bFree = false;
						pThreadData.WaitHandle.Set();
						break;
					}
				}
			}
		}
		/// <summary>
		/// create a thread pool thread belongs
		/// </summary>
		/// <returns>If the current thread pool has been created to return false, otherwise true</returns>
		public bool Create()
		{
			if (this.m_bStarted == true) return false;

			this.m_bPoolIsDestroying = false;

			m_hNotifyShutdown.Reset();

			
			for (int nIndex = 0; nIndex < this.m_nPoolSize; nIndex++)
			{
				ThreadData pThreadData = new ThreadData();

				pThreadData.bFree = true;
				pThreadData.WaitHandle = new ManualResetEvent(false);
				pThreadData.hThread = new Thread(ThreadProc);

				pThreadData.hThread.IsBackground = true;
				pThreadData.dwThreadId = nIndex + 1;

				m_threads.Add(pThreadData);

				pThreadData.hThread.Start(pThreadData);
			}

			return true;
		}

		/// <summary>
		/// Stop all the current thread pool thread
		/// </summary>
		public void Destroy()
		{
			if (this.m_bStarted == false) return;

			this.m_bPoolIsDestroying = true;

			this.m_hNotifyShutdown.Set();

			foreach (ThreadData pThreadData in m_threads)
			{
				while (pThreadData.hThread.ThreadState != ThreadState.Stopped)
				{
					Thread.Sleep(1);
				}
			}

			m_threads.Clear();

			this.m_bStarted = false;
		}
		/// <summary>
		/// gets a thread function execution
		/// </summary>
		/// <returns></returns>
		private FunctionData GetThreadProc()
		{
			FunctionData functions = null;
			lock (this)
			{
				if (m_functionList.Count > 0)
				{
					functions = m_functionList.First.Value;
					m_functionList.RemoveFirst();
				}
			}
			return functions;
		}

		/// <summary>
		/// thread pool main logic function
		/// thread scheduling management function
		/// </summary>
		/// <param name="pParam"></param>
		private void ThreadProc(object pParam)
		{
			FunctionData functions = null;
			WaitHandle[] hWaits = new WaitHandle[2];
			ThreadData pThreadData = pParam as ThreadData;

			hWaits[0] = pThreadData.WaitHandle;
			hWaits[1] = this.m_hNotifyShutdown;

			while(true)
			{
				EventWaitHandle.WaitAny(hWaits);

				if (this.m_bPoolIsDestroying == true)
				{
					break;
				}

				this.BusyNotify(pThreadData);

				while ((functions = GetThreadProc()) != null)
				{
					try
					{
						if (functions.RunObject != null)
						{
							functions.RunObject.Run();
						}
						else
						{
							functions.ThreadFunc(functions.param);
						}
					}
					catch (Exception)
					{

					}
				}

				this.FinishNotify(pThreadData);
			}
		}
	}
}
