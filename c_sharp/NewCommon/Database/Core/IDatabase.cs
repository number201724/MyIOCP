////======================================================================
////
////        Filename    : IDatabase.cs
////        Description : Database interface
////
////        Created by kesalin@gmail.com at 2013-3-24 13:26:06
////        http://blog.csdn.net/kesalin/
//// 
////======================================================================

using System.Data;

namespace NewCommon.Database.Core
{
    public interface IDatabase
    {
        IDbConnection Connection { get; }
        string ConnectionString { get; set; }

        void Open();
        void Close();
		bool IsOpen();
        void Dispose();

        void BeginTrans();
        void CommitTrans();
        void RollbackTrans();

        int ExcuteSql(string strSql);
        int ExcuteSql(string strSql, string[] strParams, object[] objValues);
        object ExcuteScalarSql(string strSql);
        object ExcuteScalarSql(string strSql, string[] strParams, object[] strValues);
        DataSet ExcuteSqlForDataSet(string queryString);
    }
}
