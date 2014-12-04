////======================================================================
////
////        Filename    : DatabaseHelper.cs
////        Description : Database helper 
////
////        Created by kesalin@gmail.com at 2013-3-24 13:26:06
////        http://blog.csdn.net/kesalin/
//// 
////======================================================================

using System;
using System.Data;
using System.Text;
using NewCommon.Database.Core;

namespace NewCommon.Database.Utility
{
    public static class DatabaseHelper
    {
        #region Connection string

        public static string CreateConnectionString(DatabaseType dbType, string dbHost, string dbName, string dbUser, string dbPwd)
        {
            switch (dbType)
            {
                case DatabaseType.SqlServer:
                    return CreateSQLServerConnectionString(dbHost, dbName, dbUser, dbPwd);

                case DatabaseType.MySQL:
                    return CreateMySQLConnectionString(dbHost, dbName, dbUser, dbPwd);

                case DatabaseType.Oracle:
                    return CreateOracleConnectionString(dbHost, dbName, dbUser, dbPwd);

                default:
                    throw new Exception("Unsupported database type.");
            }
        }

        public static string CreateSQLServerConnectionString(string dbHost, string dbName, string dbUser, string dbPwd)
        {
            var sb = new StringBuilder();
            sb.AppendFormat("Data Source={0};Initial Catalog={1};User Id={2};Password={3};", dbHost, dbName, dbUser, dbPwd);
            return sb.ToString();
        }

        public static string CreateMySQLConnectionString(string dbHost, string dbName, string dbUser, string dbPwd)
        {
            var sb = new StringBuilder();
			sb.AppendFormat("server={0};database={1};uid={2};pwd={3};Convert Zero Datetime=True;Allow Zero Datetime=True;", dbHost, dbName, dbUser, dbPwd);
            return sb.ToString();
        }

        public static string CreateOracleConnectionString(string dbHost, string dbName, string dbUser, string dbPwd)
        {
            var sb = new StringBuilder();
            sb.AppendFormat("password={0};User ID={1};Data Source=(DESCRIPTION=(ADDRESS_LIST=(ADDRESS=(PROTOCOL=TCP)(HOST={2})(PORT=1521)))(CONNECT_DATA=(SERVER=DEDICATED)(SERVICE_NAME={3})))",
                dbPwd, dbUser, dbHost, dbName);
            return sb.ToString();
        }

        #endregion

        #region DataSet helper

        public static int GetRowCount(DataSet dataset)
        {
            return dataset.Tables[0].Rows.Count;
        }

        public static int GetColumnCount(DataSet dataset)
        {
            return dataset.Tables[0].Columns.Count;
        }

        public static object GetValue(DataSet dataset, int row, string columnName)
        {
#if DEBUG
            if (row >= GetRowCount(dataset))
                throw new IndexOutOfRangeException(string.Format("Row out of bounds when get value from db dataset for {0} - row {1}", row, columnName));
#endif
            return dataset.Tables[0].Rows[row][columnName];
        }

        public static int GetIntValue(DataSet dataset, int row, int columnIndex)
        {
#if DEBUG
            if (row >= GetRowCount(dataset))
                throw new IndexOutOfRangeException(string.Format("Row out of bounds when get int value from db dataset for {0} - {1}", row, columnIndex));
            if (columnIndex >= GetColumnCount(dataset))
                throw new IndexOutOfRangeException(string.Format("Column out of bounds when get int value from db dataset for {0} - {1}", row, columnIndex));
#endif

             return Convert.ToInt32(dataset.Tables[0].Rows[row][columnIndex]);
        }

        public static object GetValue(DataSet dataset, int row, int columnIndex)
        {
#if DEBUG
            if (row >= GetRowCount(dataset))
                throw new IndexOutOfRangeException(string.Format("Row out of bounds when get value from db dataset for {0} - {1}", row, columnIndex));
            if (columnIndex >= GetColumnCount(dataset))
                throw new IndexOutOfRangeException(string.Format("Column out of bounds when get value from db dataset for {0} - {1}", row, columnIndex));
#endif
            return dataset.Tables[0].Rows[row][columnIndex];
        }

        #endregion

        #region Validate SQL command

        public static string ValidateSQLCommand(DatabaseType dbType, string sqlCmd)
        {
            if (string.IsNullOrEmpty(sqlCmd))
                return sqlCmd;

            var retVal = new StringBuilder();
            if (dbType == DatabaseType.SqlServer)
            {
                foreach (char t in sqlCmd)
                {
                    switch (t)
                    {
                        case '\'':
                            retVal.Append("''");
                            break;
                        case '"':
                            retVal.Append("\"\"");
                            break;
                        default:
                            retVal.Append(t);
                            break;
                    }
                }
            }
            else if (dbType == DatabaseType.MySQL)
            {
                foreach (char t in sqlCmd)
                {
                    switch (t)
                    {
                        case '\'':
                            retVal.Append("\\'");
                            break;
                        case '\\':
                            retVal.Append("\\\\");
                            break;
                        default:
                            retVal.Append(t);
                            break;
                    }
                }
            }
            else
            {
                // TODO:Oracle
                retVal.Append(sqlCmd);
            }

            return retVal.ToString();
        }

        #endregion
    }
}
