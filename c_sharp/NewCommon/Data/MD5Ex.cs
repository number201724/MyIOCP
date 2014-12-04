using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Security.Cryptography;
namespace NewCommon.Data
{
	public class MD5Ex
	{
		public static string getMd5Hash(byte[] input)
		{
			// Create a new instance of the MD5CryptoServiceProvider object.
			MD5CryptoServiceProvider md5Hasher = new MD5CryptoServiceProvider();

			// Convert the input string to a byte array and compute the hash.
			byte[] data = md5Hasher.ComputeHash(input);

			// Create a new Stringbuilder to collect the bytes
			// and create a string.
			StringBuilder sBuilder = new StringBuilder();

			// Loop through each byte of the hashed data 
			// and format each one as a hexadecimal string.
			for (int i = 0; i < data.Length; i++)
			{
				sBuilder.Append(data[i].ToString("x2"));
			}

			// Return the hexadecimal string.
			return sBuilder.ToString();
		}

	}
}
