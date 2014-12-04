using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Data;
using NewCommon.Database.Core;
using NewCommon.Database.Utility;
namespace NewCommon.Database.Pool
{
	/// <summary>
	/// 简单 MySQL 连接管理
	/// --每个线程ID，单独创建一个DataBase 连接
	/// --安全，免加锁，适用于有多个工作线程的情况使用
	/// </summary>
	public class AdoConManager
	{
		/// <summary>
		/// 管理连接信息的类
		/// </summary>
		private class ConnectInfo
		{
			public DatabaseType m_dbType = DatabaseType.MySQL;
			public string m_dbHost;
			public string m_dbName;
			public string m_dbUser;
			public string m_dbPwd;
			public long m_ConnectTimeOut;
		}

		private Dictionary<int, IDatabase> m_ConnectList;
		private ConnectInfo m_ConnectInfo;
		private bool m_bStarted;


		public AdoConManager()
		{
			this.m_ConnectList = new Dictionary<int, IDatabase>();
			this.m_ConnectInfo = new ConnectInfo();
			this.m_bStarted = false;
			
			this.m_ConnectInfo.m_ConnectTimeOut = 60;
		}

		public void SetConnectParam(DatabaseType dbType, string dbHost, string dbName, string dbUser, string dbPwd)
		{
			if (!m_bStarted)
			{
				m_bStarted = true;
				m_ConnectInfo.m_dbType = dbType;
				m_ConnectInfo.m_dbHost = dbHost;
				m_ConnectInfo.m_dbName = dbName;
				m_ConnectInfo.m_dbUser = dbUser;
				m_ConnectInfo.m_dbPwd = dbPwd;
			}
		}
		private IDatabase CreateDatabase()
		{
			string connStr = DatabaseHelper.CreateConnectionString(m_ConnectInfo.m_dbType, m_ConnectInfo.m_dbHost, m_ConnectInfo.m_dbName, m_ConnectInfo.m_dbUser, m_ConnectInfo.m_dbPwd);
			return DatabaseFactory.CreateDatabase(m_ConnectInfo.m_dbType, connStr);
		}
		

		private IDatabase ConnectToDB(IDatabase pDatabase)
		{
			if(pDatabase.IsOpen() == false)
			{
				try
				{
					pDatabase.Open();
				}
				catch (Exception)
				{

				}
			}
			return pDatabase;
		}
		public IDatabase GetDBConnect(int nIndex)
		{
			IDatabase pDatabase = null;
			lock (this)
			{
				if (m_ConnectList.ContainsKey(nIndex))
				{
					pDatabase = m_ConnectList[nIndex];
				}
				else
				{
					pDatabase = CreateDatabase();
					m_ConnectList[nIndex] = pDatabase;
				}
			}

			return ConnectToDB(pDatabase);
		}
		public void ClearAll()
		{
			lock (this)
			{
				foreach (IDatabase db in m_ConnectList.Values)
				{
					try
					{
						db.Close();
					}
					catch (Exception)
					{
					}
				}
				m_ConnectList.Clear();
			}
		}
	}
}
