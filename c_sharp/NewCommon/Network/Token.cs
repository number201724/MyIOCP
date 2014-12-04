using System;
using System.Collections.Generic;
using System.Text;
using System.Net.Sockets;
using System.Globalization;

namespace NewCommon.Network
{
	delegate void ProcessData(SocketAsyncEventArgs args);

	/// <summary>
	/// Token for use with SocketAsyncEventArgs.
	/// </summary>
	public class IocpUserToken
	{
		private Socket connection;
		private long unique_id;
		public object user_object = null;
		/// <summary>
		/// Class constructor.
		/// </summary>
		/// <param name="connection">Socket to accept incoming data.</param>
		public IocpUserToken(Socket connection, long unique_id)
		{
			this.connection = connection;
			this.unique_id = unique_id;
		}

		/// <summary>
		/// Accept socket.
		/// </summary>
		public Socket Connection
		{
			get { return this.connection; }
		}
		public long UniqueId
		{
			get { return this.unique_id; }
		}
	}
}
