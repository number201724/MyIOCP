using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Data;
namespace NewCommon.Database.Core
{
	public class ADORecordset : IDisposable
	{
		private IDatabase instance;
		private DataTable fields;
		private int curIndex = -1;
		public ADORecordset(IDatabase pDatabse)
		{
			this.instance = pDatabse;
		}
		public void Dispose()
		{

		}

		public bool Open(string command)
		{
			try
			{
				DataSet mySet = this.instance.ExcuteSqlForDataSet(command);
				fields = mySet.Tables[0];
				return true;
			}
			catch (Exception)
			{
				return false;
			}
		}

		public bool IsOpen()
		{
			return (fields != null);
		}
		public bool MoveNext()
		{
			if (fields.Rows.Count == 0) return false;

			curIndex++;

			if (curIndex >= fields.Rows.Count) return false;
			return true;
		}

		public bool IsEOF()
		{
			return curIndex == fields.Rows.Count;
		}

		public int GetRecordCount()
		{
			if (fields == null) return 0;
			return fields.Rows.Count;
		}

		public void GetFieldValue(string key,out string value)
		{
			value = "";

			if (fields.Rows[curIndex][key].GetType() == typeof(System.String))
			{
				value = (string)fields.Rows[curIndex][key];
			}
		}
		public void GetFieldValue(string key, out Int32 value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.Int32))
			{
				value = (Int32)fields.Rows[curIndex][key];
			}
		}

		public void GetFieldValue(string key, out UInt32 value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.UInt32))
			{
				value = (UInt32)fields.Rows[curIndex][key];
			}
		}

		public void GetFieldValue(string key, out Int64 value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.Int64))
			{
				value = (Int64)fields.Rows[curIndex][key];
			}
		}

		public void GetFieldValue(string key, out UInt64 value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.UInt64))
			{
				value = (UInt64)fields.Rows[curIndex][key];
			}
		}
		public void GetFieldValue(string key, out Int16 value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.Int16))
			{
				value = (Int16)fields.Rows[curIndex][key];
			}
		}

		public void GetFieldValue(string key, out UInt16 value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.UInt16))
			{
				value = (UInt16)fields.Rows[curIndex][key];
			}
		}

		public void GetFieldValue(string key, out SByte value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.SByte))
			{
				value = (SByte)fields.Rows[curIndex][key];
			}
		}

		public void GetFieldValue(string key, out Double value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.Double))
			{
				value = (Double)fields.Rows[curIndex][key];
			}
		}

		public void GetFieldValue(string key, out Single value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.Single))
			{
				value = (Single)fields.Rows[curIndex][key];
			}
		}

		public void GetFieldValue(string key, out Decimal value)
		{
			value = 0;

			if (fields.Rows[curIndex][key].GetType() == typeof(System.Decimal))
			{
				value = (Decimal)fields.Rows[curIndex][key];
			}
		}

		public void GetFieldValue(string key, out Byte[] value)
		{
			value = new Byte[0];

			if (fields.Rows[curIndex][key].GetType() == typeof(System.Byte[]))
			{
				value = (System.Byte[])fields.Rows[curIndex][key];
			}
		}

		public object GetFieldValue(string key)
		{
			return fields.Rows[curIndex][key];
		}

		public string this[string key]
		{
			get
			{
				return fields.Rows[curIndex][key].ToString();
			}
		}
	}
}
