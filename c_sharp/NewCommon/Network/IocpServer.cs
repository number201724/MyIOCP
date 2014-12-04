using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.IO;
using NewCommon.Data;
namespace NewCommon.Network
{
	public class IocpPacketEventArgs : EventArgs
	{
		public IocpUserToken token;
		public byte[] Buffer;
		public IocpPacketEventArgs(IocpUserToken token, byte[] buffer)
		{
			this.token = token;
			this.Buffer = buffer;
		}
	}
	public class IocpNewConnectionArgs : EventArgs
	{
		public IocpUserToken token;
		public IocpNewConnectionArgs(IocpUserToken token)
		{
			this.token = token;
		}
	}
	public class IocpDisconnectConnectionArgs : EventArgs
	{
		public IocpUserToken token;

		public IocpDisconnectConnectionArgs(IocpUserToken token)
		{
			this.token = token;
		}
	}

	public class IocpServer
	{
		//定义一下委托事件
		public delegate void DelegateNotifyNewConnection(object sender, IocpNewConnectionArgs e);
		public delegate void DelegateNotifyReceivedPackage(object sender, IocpPacketEventArgs e);
		public delegate void DelegateNotifyDisconnectedConnection(object sender, IocpDisconnectConnectionArgs e);
		public delegate void DelegateNotifySendComplete(object sender, IocpPacketEventArgs e);
		/// <summary>
		/// 接收用户连接的socket。
		/// </summary>
		private Socket listenSocket;

		/// <summary>
		/// I/O 缓冲区大小。
		/// </summary>
		private static Int32 bufferSize = 8192;

		/// <summary>
		/// 当前总共有多少个连接。
		/// </summary>
		public Int32 numConnectedSockets;

		/// <summary>
		/// 服务器最大接收多少连接数。
		/// </summary>
		public Int32 numConnections;

		/// <summary>
		/// SocketAsyncEventArgs结构池,方便快速的Accept,Read,Write。
		/// </summary>
		private SocketAsyncEventArgsPool readWritePool;

		/// <summary>
		/// 服务器的唯一ID,用来管理客户多线程发送数据冲突的问题。
		/// </summary>
		private long unique_id = 0;

		/// <summary>
		/// 管理用户唯一ID对应的用户IocpUserToken结构表。
		/// </summary>
		private Dictionary<long, IocpUserToken> tokenMap;

		/// <summary>
		/// 有新用户连接的事件。
		/// </summary>
		public event DelegateNotifyNewConnection NotifyNewConnection;

		/// <summary>
		/// 有用户断开的事件。
		/// </summary>
		public event DelegateNotifyDisconnectedConnection NotifyDisconnectConnection;

		/// <summary>
		/// 有数据包到达服务器的事件。
		/// </summary>
		public event DelegateNotifyReceivedPackage NotifyReceivedPackage;

		/// <summary>
		/// 有数据发送完成的事件。
		/// </summary>
		public event DelegateNotifySendComplete NotifySendComplete;

		/// <summary>
		/// 申请一个SocketAsyncEventArgs结构。
		/// </summary>
		/// <returns></returns>
		private SocketAsyncEventArgs AllocSocketAsyncEventArgs()
		{
			SocketAsyncEventArgs readWriteEventArg = new SocketAsyncEventArgs();

			readWriteEventArg.Completed += new EventHandler<SocketAsyncEventArgs>(OnIOCompleted);
			readWriteEventArg.SetBuffer(new Byte[bufferSize], 0, bufferSize);

			return readWriteEventArg;
		}

		/// <summary>
		/// 从内存池中获取一个SocketAsyncEventArgs结构。
		/// </summary>
		/// <returns></returns>
		private SocketAsyncEventArgs GetSocketAsyncEventArgs()
		{
			SocketAsyncEventArgs readWriteEventArg = null;
			lock (this.readWritePool)
			{
				readWriteEventArg = this.readWritePool.Pop();
				if (readWriteEventArg == null)
				{
					readWriteEventArg = this.AllocSocketAsyncEventArgs();
				}
			}
			return readWriteEventArg;
		}
		/// <summary>
		/// 释放一个SocketAsyncEventArgs结构到内存池中。
		/// </summary>
		/// <param name="args"></param>
		private void ReleaseSocketAsyncEventArgs(SocketAsyncEventArgs args)
		{
			lock (this.readWritePool)
			{
				this.readWritePool.Push(args);
			}
		}

		/// <summary>
		/// 构造函数。
		/// 初始化连接最大数量，内存池等结构。
		/// </summary>
		public IocpServer(int numConnections)
		{
			this.numConnectedSockets = 0;
			this.numConnections = numConnections;

			this.readWritePool = new SocketAsyncEventArgsPool(3000);
			this.tokenMap = new Dictionary<long, IocpUserToken>();

			for (Int32 i = 0; i < (this.numConnections * 2); i++)
			{
				// 申请一个SocketAsyncEventArgs结构，并添加到内存池。
				this.readWritePool.Push(this.AllocSocketAsyncEventArgs());
			}
		}

		/// <summary>
		/// 关闭一个连接。
		/// </summary>
		/// <param name="e">与发送接收相关的SocketAsyncEventArgs结构</param>
		public void CloseClientSocket(SocketAsyncEventArgs e)
		{
			IocpUserToken token = e.UserToken as IocpUserToken;
			this.CloseClientSocket(token, e);
		}

		/// <summary>
		/// 关闭一个连接。
		/// </summary>
		/// <param name="token">用户关联的IocpUserToken结构</param>
		/// <param name="e">与发送接收相关的SocketAsyncEventArgs结构</param>
		private void CloseClientSocket(IocpUserToken token, SocketAsyncEventArgs e)
		{
			lock (tokenMap)
			{
				if (tokenMap.Remove(token.UniqueId))
				{
					//通知用户断开了连接
					if (NotifyDisconnectConnection != null)
					{
						NotifyDisconnectConnection(this, new IocpDisconnectConnectionArgs(token));
					}

					// 减少总连接数
					Interlocked.Decrement(ref this.numConnectedSockets);

					token.Connection.Close();
				}
			}

			// 释放SocketAsyncEventArgs结构,以方便让其他套接字重用该结构
			this.ReleaseSocketAsyncEventArgs(e);
		}


		private void ProcessError(SocketAsyncEventArgs e)
		{
			IocpUserToken token = e.UserToken as IocpUserToken;
			this.CloseClientSocket(token, e);
		}

		/// <summary>
		/// 这种方法被调用时，一个异步接收操作完成。
		/// 分发派遣不同的IO操作函数。
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void OnIOCompleted(object sender, SocketAsyncEventArgs e)
		{
			switch (e.LastOperation)
			{
				case SocketAsyncOperation.Accept:
					this.ProcessAccept(e);
					break;
				case SocketAsyncOperation.Receive:
					this.OnClientIoReceive(e);
					break;
				case SocketAsyncOperation.Send:
					this.OnClientIoSend(e);
					break;
			}
		}

		/// <summary>
		/// 当这个被调用时，代表一个异步接收操作完成。
		/// 当远程主机关闭连接，这个Socket也将被关闭
		/// 如果有数据接收，那么就会执行事件通知用户
		/// </summary>
		/// <param name="e">SocketAsyncEventArg表示一个已经完成的异步IO事件。</param>
		private void OnClientIoReceive(SocketAsyncEventArgs e)
		{
			//获取与用户关联的Token结构
			IocpUserToken token = e.UserToken as IocpUserToken;
			//获取用户的套接字
			Socket s = token.Connection;
			// 检查是否是数据接收
			// 是否非错误并且SOCKET是有效的

			try
			{
				if (e.BytesTransferred > 0 && e.SocketError == SocketError.Success && s.Available == 0)
				{
					if (NotifyReceivedPackage != null)
					{
						MemoryStream TempBuffer = new MemoryStream();
						new BinaryWriter(TempBuffer).Write(e.Buffer, 0, e.BytesTransferred);

						NotifyReceivedPackage(this, new IocpPacketEventArgs(token, BufferEx.MemoryStreamToBytes(TempBuffer)));
					}
					//投递下一个接收函数
					if (!s.ReceiveAsync(e))
					{
						// 如果投递失败,检查是否是成功接收
						this.OnClientIoReceive(e);
					}
				}
				else
				{
					throw new SocketException(10054);//WSAECONNRESET 
				}
			}
			catch (Exception)
			{
				this.CloseClientSocket(e);
			}
		}
		/// <summary>
		/// 当这个被调用时，代表一个异步接收操作完成。
		/// 当远程主机关闭连接，这个Socket也将被关闭
		/// 如果没有错误,那么就会执行事件通知用户发送的字节数以及数据
		/// </summary>
		/// <param name="e">SocketAsyncEventArg表示一个已经完成的异步IO事件。</param>
		private void OnClientIoSend(SocketAsyncEventArgs e)
		{
			//获取与用户关联的Token结构
			IocpUserToken token = e.UserToken as IocpUserToken;
			//获取用户的套接字
			Socket s = token.Connection;

			if (e.BytesTransferred > 0 && e.SocketError == SocketError.Success)
			{
				if (NotifySendComplete != null)
				{
					MemoryStream TempBuffer = new MemoryStream();
					new BinaryWriter(TempBuffer).Write(e.Buffer, 0, e.BytesTransferred);
					NotifySendComplete(this, new IocpPacketEventArgs(token, BufferEx.MemoryStreamToBytes(TempBuffer)));
				}

				ReleaseSocketAsyncEventArgs(e);
			}
			else
			{
				this.CloseClientSocket(e);
			}
		}
		/// <summary>
		/// 当这个被调用时，代表一个异步接收操作完成。
		/// 处理新用户的连接请求。
		/// </summary>
		/// <param name="e">SocketAsyncEventArg表示一个已经完成的异步IO事件。</param>
		private void ProcessAccept(SocketAsyncEventArgs e)
		{
			Socket s = e.AcceptSocket;
			if (s.Connected)
			{
				SocketAsyncEventArgs readEventArgs = this.GetSocketAsyncEventArgs();
				try
				{
					IocpUserToken token = new IocpUserToken(s, Interlocked.Increment(ref unique_id));
					readEventArgs.UserToken = token;

					//增加当前服务器的连接数量计数器。
					Interlocked.Increment(ref this.numConnectedSockets);

					//添加用户连接到表
					lock (tokenMap)
					{
						tokenMap[token.UniqueId] = token;
					}

					if (NotifyNewConnection != null)
					{
						NotifyNewConnection(this, new IocpNewConnectionArgs(token));
					}

					if (!s.ReceiveAsync(readEventArgs))
					{
						this.OnClientIoReceive(readEventArgs);
					}
				}
				catch (Exception)
				{
					CloseClientSocket(readEventArgs);
				}

				// 接收下一个连接请求
				this.PostIoAccept(e);
			}
			else
			{
				s.Close();
			}
		}


		/// <summary>
		/// 投递一个接收请求。
		/// </summary>
		/// <param name="acceptEventArg">表示一个IO接收事件，如果为空，则申请一个io事件</param>
		private void PostIoAccept(SocketAsyncEventArgs acceptEventArg)
		{
			if (acceptEventArg == null)
			{
				acceptEventArg = new SocketAsyncEventArgs();
				acceptEventArg.Completed += new EventHandler<SocketAsyncEventArgs>(OnIOCompleted);
			}
			else
			{
				// 套接字必须清除，因为接收结构被重用
				acceptEventArg.AcceptSocket = null;
			}
			try
			{
				if (!this.listenSocket.AcceptAsync(acceptEventArg))
				{
					this.ProcessAccept(acceptEventArg);
				}
			}
			catch (Exception)
			{
				if (acceptEventArg.AcceptSocket != null)
				{
					acceptEventArg.AcceptSocket.Close();
				}
			}
		}

		/// <summary>
		/// 启动服务器,并且等待用户连接请求
		/// </summary>
		/// <param name="port">服务器监听的端口</param>
		public void Start(Int32 port, Int32 acceptIoCount)
		{
			// 创建一个监听用的套接字
			this.listenSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

			//关闭发送缓冲区,这可以减少一次内存拷贝的消耗
			this.listenSocket.SendBufferSize = 0;


			// 关联套接字到端口
			this.listenSocket.Bind(new IPEndPoint(IPAddress.Any, port));

			// 启动服务器监听
			this.listenSocket.Listen(this.numConnections);

			//投递多个接收请求,服务器启动完成
			for (Int32 i = 0; i < acceptIoCount; i++)
			{
				this.PostIoAccept(null);
			}
		}

		/// <summary>
		/// 停止服务器，并断开所有用户
		/// </summary>
		public void Stop()
		{
			this.listenSocket.Close();

			lock (tokenMap)
			{
				foreach (IocpUserToken token in tokenMap.Values)
				{
					token.Connection.Close();
				}
			}

			while (this.numConnectedSockets != 0)
				Thread.Sleep(1);

			this.listenSocket = null;
		}

		/// <summary>
		/// 发送数据包给客户
		/// </summary>
		/// <param name="token"></param>
		/// <param name="sendBuffer"></param>
		public void RealSend(IocpUserToken token, byte[] sendBuffer)
		{
			SocketAsyncEventArgs args = GetSocketAsyncEventArgs();
			args.SetBuffer(sendBuffer, 0, sendBuffer.Length);
			args.UserToken = token;
			try
			{
				if (!token.Connection.SendAsync(args))
				{
					OnClientIoSend(args);
				}
			}
			catch (Exception)
			{
				CloseClientSocket(args);        //关闭一个套接字
			}
		}
	}
}
