using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using NewCommon.Data;
namespace NewCommon.Network
{
	public class NetworkServer : IocpServer
	{
		public class NetworkComputeBuffer
		{
			public int BytesTransferred;
			public MemoryStream TotalBuffer;
			public IocpUserToken token;

			public NetworkComputeBuffer(IocpUserToken _instance)
			{
				this.token = _instance;
				BytesTransferred = 0;
				TotalBuffer = new MemoryStream();
			}

			public BinaryReader GetReader()
			{
				return new BinaryReader(BufferEx.MemoryStreamToReaderStream(this.TotalBuffer));
			}
		};

		public class USER_NetworkReceivedArgs : EventArgs
		{
			public IocpUserToken token;
			public byte[] buffer;

			public USER_NetworkReceivedArgs(IocpUserToken token, byte[] buffer)
			{
				this.token = token;
				this.buffer = buffer;
			}
		}

		public delegate void DelegateUSERNotifyReceivedPackage(object sender, USER_NetworkReceivedArgs e);

		private Dictionary<long, NetworkComputeBuffer> ComputeTable;

		public event DelegateUSERNotifyReceivedPackage USERNotifyReceivedPackage;

		public NetworkServer(int numConnections)
			: base(numConnections)
		{
			ComputeTable = new Dictionary<long, NetworkComputeBuffer>();
			base.NotifyNewConnection += new DelegateNotifyNewConnection(NetworkServer_NotifyNewConnection);
			base.NotifyDisconnectConnection += new DelegateNotifyDisconnectedConnection(NetworkServer_NotifyDisconnectConnection);
			base.NotifyReceivedPackage += new DelegateNotifyReceivedPackage(NetworkServer_NotifyReceivedPackage);
			base.NotifySendComplete += new DelegateNotifySendComplete(NetworkServer_NotifySendComplete);
		}

		void NetworkServer_NotifySendComplete(object sender, IocpPacketEventArgs e)
		{

		}

		void NetworkServer_NotifyReceivedPackage(object sender, IocpPacketEventArgs e)
		{
			NetworkComputeBuffer ComputeBuffer = null;
			lock (ComputeTable)
			{
				if (ComputeTable.ContainsKey(e.token.UniqueId))
				{
					ComputeBuffer = ComputeTable[e.token.UniqueId];
				}
			}

			//附加数据到缓冲区
			ComputeBuffer.TotalBuffer.Write(e.Buffer, 0, e.Buffer.Length);

			//开始拆分封包数据
			while (ComputeBuffer.TotalBuffer.Length >= Shared.GetPackageInformationLength())      //封包是否符合逻辑
			{
				//开始读取封包
				BinaryReader ComputeReader = ComputeBuffer.GetReader();

				uint CurrentBlockLength = ComputeReader.ReadUInt32();       //当前数据分块的长度
				ushort CurrentPackageSignature = ComputeReader.ReadUInt16();       //封包标记

				if (
					CurrentBlockLength <= 0 ||  //包大小是否负数
					CurrentBlockLength > 0x100000 || //包大小大于1MB
					CurrentBlockLength < Shared.GetPackageInformationLength() || //包头的大小
					CurrentPackageSignature != Shared.PackageSignature     //包标记
					)
				{
					throw new BadPackageException();
				}

				if (CurrentBlockLength > ComputeBuffer.TotalBuffer.Length) //是否完整的数据,就等待客户端下次发送数据到达
				{
					break;
				}

				int CurrentBlockDataLength = (int)CurrentBlockLength - Shared.GetPackageInformationLength();

				//解密数据
				byte[] encrypted = ComputeReader.ReadBytes(CurrentBlockDataLength);
				byte[] decrypted = AESEx.AESDecrypt(encrypted, CommonConfig.key, CommonConfig.iv);


				if (USERNotifyReceivedPackage != null)
				{
					USERNotifyReceivedPackage(this, new USER_NetworkReceivedArgs(e.token, decrypted));
				}

				int nextBufferLength = (int)(ComputeBuffer.TotalBuffer.Length - CurrentBlockLength);
				ComputeBuffer.TotalBuffer.SetLength(0);
				ComputeBuffer.TotalBuffer.Write(ComputeReader.ReadBytes(nextBufferLength), 0, nextBufferLength);
			}
		}

		void NetworkServer_NotifyDisconnectConnection(object sender, IocpDisconnectConnectionArgs e)
		{
			lock (ComputeTable)
			{
				ComputeTable.Remove(e.token.UniqueId);
			}
		}

		void NetworkServer_NotifyNewConnection(object sender, IocpNewConnectionArgs e)
		{
			lock (ComputeTable)
			{
				ComputeTable[e.token.UniqueId] = new NetworkComputeBuffer(e.token);
			}
		}

		public void SendEx(IocpUserToken token, byte[] buffer)
		{
			byte[] encrypted = AESEx.AESEncrypt(buffer, CommonConfig.key, CommonConfig.iv);

			MemoryStream st = new MemoryStream();
			BinaryWriter bw = new BinaryWriter(st);

			bw.Write(encrypted.Length + sizeof(int) + sizeof(ushort));
			bw.Write(Shared.PackageSignature);
			bw.Write(encrypted);
			RealSend(token, BufferEx.MemoryStreamToBytes(st));
		}
	}
}
