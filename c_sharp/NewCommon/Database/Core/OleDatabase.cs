////======================================================================
////
////        Filename    : OleDatabase.cs
////        Description : Database for OLE
////
////        Created by kesalin@gmail.com at 2013-3-24 13:26:06
////        http://blog.csdn.net/kesalin/
//// 
////======================================================================

using System;
using System.Data;
using System.Data.OleDb;

namespace NewCommon.Database.Core
{
    public class OleDatabase : IDatabase
    {
        #region Private fields
        private OleDbConnection _conn;
        private OleDbTransaction _trans;
        private bool _isInTransaction;
        private string _connectionString;
        #endregion

        #region Public function
        public OleDatabase()
        {
        }

        public OleDatabase(string connectionString)
        {
            if (string.IsNullOrEmpty(connectionString))
                throw new ArgumentNullException("connectionString");

            _connectionString = connectionString;

            _conn = new OleDbConnection(_connectionString);
            if (_conn == null)
            {
                throw new Exception(
                    string.Format("Failed to create OleDbConnection with connectionString: {0}", _connectionString));
            }
        }

        private string GetConnectionState()
        {
            if (_conn == null)
            {
                return null;
            }

            return _conn.State.ToString().ToUpper();
        }

        private bool IsConnectionOpened()
        {
            var state = GetConnectionState();
            return (state != null && state == "OPEN");
        }
        #endregion

        #region IDatabase interface
        public IDbConnection Connection
        {
            get { return _conn; }
        }

        public string ConnectionString
        {
            get { return _connectionString; }
            set
            {
                if (string.IsNullOrEmpty(value))
                    throw new ArgumentException("ConnectionString is null!");

                _connectionString = value;
            }
        }

        public void Open()
        {
            if (_conn == null)
            {
                if (string.IsNullOrEmpty(_connectionString))
                {
                    throw new Exception("Cannot create OleDbConnection with null connectionString!");
                }

                _conn = new OleDbConnection(_connectionString);
                if (_conn == null)
                {
                    throw new Exception(
                        string.Format("Failed to create OleDbConnection with connectionString: {0}", _connectionString));
                }
            }

            if (!IsConnectionOpened())
            {
                _conn.Open();
            }
        }

        public void Close()
        {
            if (_conn == null)
                return;

            if (IsConnectionOpened())
            {
                _conn.Close();
            }
        }

		public bool IsOpen()
		{
			if (_conn == null)
				return false;
			return IsConnectionOpened();
		}

        public void Dispose()
        {
            if (_conn == null)
                return;

            Close();

            _conn.Dispose();
            _conn = null;
        }

        public void BeginTrans()
        {
            if (_conn != null)
            {
                if (_trans != null)
                {
                    throw new Exception("Transition already began! Please commit it before begin a new one.");
                }

                _trans = _conn.BeginTransaction();
                _isInTransaction = true;
            }
        }

        public void CommitTrans()
        {
            if (_trans != null)
            {
                _trans.Commit();
                _isInTransaction = false;
            }
        }

        public void RollbackTrans()
        {
            if (_trans != null)
            {
                _trans.Rollback();
                _isInTransaction = false;
            }
        }

        public int ExcuteSql(string sqlCmd)
        {
            return ExcuteSql(sqlCmd, null, null);
        }

        public int ExcuteSql(string sqlCmd, string[] strParams, object[] strValues)
        {
            if (string.IsNullOrEmpty(sqlCmd))
                throw new ArgumentNullException("sqlCmd");

            if ((strParams == null && strValues != null)
                || (strParams != null && strValues == null)
                || (strParams != null && strParams.Length != strValues.Length))
                throw new ArgumentException(
                    string.Format("Parameters do not match values when excuting SQL {0}.!", sqlCmd));

            var cmd = new OleDbCommand { Connection = _conn, CommandText = sqlCmd };

            if (_isInTransaction)
                cmd.Transaction = _trans;

            if (strParams != null)
            {
                for (int i = 0; i < strParams.Length; i++)
                    cmd.Parameters.AddWithValue(strParams[i], strValues[i]);
            }

            return cmd.ExecuteNonQuery();
        }

        public object ExcuteScalarSql(string sqlCmd)
        {
            return ExcuteSql(sqlCmd, null, null);
        }

        public object ExcuteScalarSql(string sqlCmd, string[] strParams, object[] strValues)
        {
            if (string.IsNullOrEmpty(sqlCmd))
                throw new ArgumentNullException("sqlCmd");

            if ((strParams == null && strValues != null)
                || (strParams != null && strValues == null)
                || (strParams != null && strParams.Length != strValues.Length))
                throw new ArgumentException(
                    string.Format("Parameters do not match values when excuting SQL {0}.!", sqlCmd));

            var cmd = new OleDbCommand { Connection = _conn, CommandText = sqlCmd };

            if (_isInTransaction)
                cmd.Transaction = _trans;

            if (strParams != null)
            {
                for (int i = 0; i < strParams.Length; i++)
                    cmd.Parameters.AddWithValue(strParams[i], strValues[i]);
            }

            return cmd.ExecuteScalar();
        }

        public DataSet ExcuteSqlForDataSet(string sqlCmd)
        {
            var cmd = new OleDbCommand { Connection = _conn, CommandText = sqlCmd };
            if (_isInTransaction)
                cmd.Transaction = _trans;
            var ds = new DataSet();
            var ad = new OleDbDataAdapter { SelectCommand = cmd };
            ad.Fill(ds);

            return ds;
        }

        #endregion
    }
}
