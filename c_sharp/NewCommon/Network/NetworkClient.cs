using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net.Sockets;
using System.IO;
using System.Runtime.InteropServices;
using NewCommon.Data;
namespace NewCommon.Network
{
	public class NetworkClient
	{
		public class NetworkComputeBuffer
		{
			public byte[] CurrentBuffer;
			public int BytesTransferred;
			public NetworkStream SocketStream;
			public MemoryStream TotalBuffer;

			public NetworkComputeBuffer(TcpClient _instance)
			{
				CurrentBuffer = new byte[8192];
				BytesTransferred = 0;
				SocketStream = _instance.GetStream();
				TotalBuffer = new MemoryStream();
			}

			public BinaryReader GetReader()
			{
				return new BinaryReader(BufferEx.MemoryStreamToReaderStream(this.TotalBuffer));
			}
		};

		public class PacketEventArgs : EventArgs
		{
			public byte[] Buffer;
			public PacketEventArgs(byte[] buffer)
			{
				this.Buffer = buffer;
			}
		};

		public class DisconnectEventArgs : EventArgs
		{
			public enum ReasonType
			{
				BadPackage,
				ReaderException,
				RemoteDisconnected,
			};
			public ReasonType reason;
			public DisconnectEventArgs(ReasonType reason)
			{
				this.reason = reason;
			}
		}

		public delegate void DelegateNotifyReceivedPackage(object sender, PacketEventArgs e);
		public delegate void DelegateNotifyDisconnectedConnection(object sender, DisconnectEventArgs e);
		public event DelegateNotifyReceivedPackage NotifyReceivedPackage;
		public event DelegateNotifyDisconnectedConnection NotifyDisconnectedConnection;
		private TcpClient _instance;

		public NetworkClient()
		{
			this._instance = new TcpClient();
		}

		private void SendComplete(IAsyncResult ar)
		{
			TcpClient mcl = (TcpClient)ar.AsyncState;
			NetworkStream st = mcl.GetStream();

			try
			{
				st.EndWrite(ar);
			}
			catch (Exception)
			{
				st.Close();
			}
		}
		private bool RealSend(byte[] buffers)
		{
			if (_instance.Connected == false) return false;

			NetworkStream st = _instance.GetStream();
			try
			{
				st.BeginWrite(buffers, 0, buffers.Length, new AsyncCallback(SendComplete), _instance);
				return true;
			}
			catch (Exception)
			{
				return false;
			}
		}

		public bool Send(byte[] buffer)
		{
			byte[] encrypted = AESEx.AESEncrypt(buffer, CommonConfig.key, CommonConfig.iv);

			MemoryStream st = new MemoryStream();
			BinaryWriter bw = new BinaryWriter(st);

			bw.Write(encrypted.Length + sizeof(int) + sizeof(ushort));
			bw.Write(Shared.PackageSignature);
			bw.Write(encrypted);

			return RealSend(BufferEx.MemoryStreamToBytes(st));
		}
		private void AsyncReadCallBack(IAsyncResult ar)
		{
			NetworkComputeBuffer ComputeBuffer = ar.AsyncState as NetworkComputeBuffer;
			try
			{
				ComputeBuffer.BytesTransferred = ComputeBuffer.SocketStream.EndRead(ar);
				if (ComputeBuffer.BytesTransferred > 0)
				{
					//附加数据到缓冲区
					ComputeBuffer.TotalBuffer.Write(ComputeBuffer.CurrentBuffer, 0, ComputeBuffer.BytesTransferred);

					//开始拆分封包数据
					while (ComputeBuffer.TotalBuffer.Length >= Marshal.SizeOf(typeof(Shared.PackageInformation)))      //封包是否符合逻辑
					{
						//开始读取封包
						BinaryReader ComputeReader = ComputeBuffer.GetReader();

						uint CurrentBlockLength = ComputeReader.ReadUInt32();       //当前数据分块的长度
						ushort CurrentPackageSignature = ComputeReader.ReadUInt16();       //封包标记

						if (
							CurrentBlockLength <= 0 ||  //包大小是否负数
							CurrentBlockLength > 0x100000 || //包大小大于1MB
							CurrentBlockLength < Marshal.SizeOf(typeof(Shared.PackageInformation)) || //包头的大小
							CurrentPackageSignature != Shared.PackageSignature     //包标记
							)
						{
							throw new BadPackageException();
						}

						if (CurrentBlockLength > ComputeBuffer.TotalBuffer.Length) //是否完整的数据,就等待客户端下次发送数据到达
						{
							break;
						}

						int CurrentBlockDataLength = (int)CurrentBlockLength - Marshal.SizeOf(typeof(Shared.PackageInformation));

						//解密数据
						byte[] encrypted = ComputeReader.ReadBytes(CurrentBlockDataLength);
						byte[] decrypted = AESEx.AESDecrypt(encrypted, CommonConfig.key, CommonConfig.iv);


						if (NotifyReceivedPackage != null)
						{
							NotifyReceivedPackage(this, new PacketEventArgs(decrypted));
						}


						int nextBufferLength = (int)(ComputeBuffer.TotalBuffer.Length - CurrentBlockLength);
						ComputeBuffer.TotalBuffer.SetLength(0);
						ComputeBuffer.TotalBuffer.Write(ComputeReader.ReadBytes(nextBufferLength), 0, nextBufferLength);
					}

					//读取下一个
					ComputeBuffer.SocketStream.BeginRead(ComputeBuffer.CurrentBuffer, 0, ComputeBuffer.CurrentBuffer.Length, new AsyncCallback(AsyncReadCallBack), ComputeBuffer);
				}
				else
				{

					if (NotifyDisconnectedConnection != null)
					{
						NotifyDisconnectedConnection(this, new DisconnectEventArgs(DisconnectEventArgs.ReasonType.ReaderException));
					}

					_instance.Close();
				}
			}
			catch (BadPackageException)
			{
				if (NotifyDisconnectedConnection != null)
				{
					NotifyDisconnectedConnection(this, new DisconnectEventArgs(DisconnectEventArgs.ReasonType.ReaderException));
				}
				_instance.Close();
			}
			catch (Exception)
			{
				if (NotifyDisconnectedConnection != null)
				{
					NotifyDisconnectedConnection(this, new DisconnectEventArgs(DisconnectEventArgs.ReasonType.ReaderException));
				}
				_instance.Close();
			};
		}
		public bool IsConnected()
		{
			return _instance.Connected;
		}
		public bool Connect(string host, int port)
		{
			try
			{
				_instance.Connect(host, port);

				NetworkComputeBuffer ComputeBuffer = new NetworkComputeBuffer(_instance);

				ComputeBuffer.SocketStream.BeginRead(ComputeBuffer.CurrentBuffer, 0, ComputeBuffer.CurrentBuffer.Length, new AsyncCallback(AsyncReadCallBack), ComputeBuffer);
			}
			catch (Exception)
			{
				return false;
			}

			return true;
		}
	}
}
