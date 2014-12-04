using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NewCommon.Data
{
	public class ConvertEx
	{
		public static string BytesToString(byte[] sb)
		{
			string temp = "";
			foreach (byte b in sb)
			{
				temp += String.Format("{0:X2}", b);
			}
			return temp;
		}

		public static string BytesToBytesString(byte[] sb)
		{
			string temp = "";
			foreach (byte b in sb)
			{
				temp += String.Format("0x{0:X2},", b);
			}


			return temp;
		}

		public static byte[] StringToBytes(string hexString)
		{
			if (hexString.Length % 2 != 0)
			{
				throw new ArgumentException();  //抛出异常
			}
			byte[] returnBytes = new byte[hexString.Length / 2];
			for (int i = 0; i < returnBytes.Length; i++)
				returnBytes[i] = Convert.ToByte(hexString.Substring(i * 2, 2), 16);
			return returnBytes;
		}

		public static string StringToHexString(string s)
		{
			string temp = "";
			byte[] encodedBytes = Encoding.UTF8.GetBytes(s);
			foreach (byte b in encodedBytes)
			{
				temp += String.Format("{0:X2}", b);
			}
			return temp;
		}
		public static string HexStringToString(string hs)
		{
			try
			{
				byte[] b = StringToBytes(hs);
				return Encoding.UTF8.GetString(b);
			}
			catch (ArgumentException)
			{
				return "";
			};
		}
	}
}
