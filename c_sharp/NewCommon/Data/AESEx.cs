using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Security.Cryptography;
using System.IO;
namespace NewCommon.Data
{
	public class AESEx
	{
		public static byte[] AESEncrypt(byte[] buffer, byte[] Key, byte[] IV)
		{
			byte[] encrypted;


			AesCryptoServiceProvider aes = new AesCryptoServiceProvider();

			aes.IV = IV;
			aes.Key = Key;

			ICryptoTransform encryptor = aes.CreateEncryptor(aes.Key, aes.IV);
			using (MemoryStream msEncrypt = new MemoryStream())
			{
				using (CryptoStream csEncrypt = new CryptoStream(msEncrypt, encryptor, CryptoStreamMode.Write))
				{
					csEncrypt.Write(buffer, 0, buffer.Length);
				}

				encrypted = msEncrypt.ToArray();
			}
			return encrypted;
		}

		public static byte[] AESDecrypt(byte[] buffer, byte[] Key, byte[] IV)
		{
			byte[] decrypted;
			
			AesCryptoServiceProvider aesAlg = new AesCryptoServiceProvider();

			aesAlg.IV = IV;
			aesAlg.Key = Key;

			ICryptoTransform decryptor = aesAlg.CreateDecryptor(aesAlg.Key, aesAlg.IV);

			using (MemoryStream msDecrypt = new MemoryStream(buffer))
			{
				using (CryptoStream csDecrypt = new CryptoStream(msDecrypt, decryptor, CryptoStreamMode.Write))
				{
					csDecrypt.Write(buffer, 0, buffer.Length);
				}

				decrypted = msDecrypt.ToArray();
			}
			return decrypted;
		}

		public static byte[] AESBuildKey(byte[] key, string append_key)
		{
			MemoryStream MemoryBuffer = new MemoryStream();
			MemoryBuffer.Write(key, 0, key.Length);

			byte[] append_bytes = Encoding.Default.GetBytes(append_key);

			MemoryBuffer.Write(append_bytes, 0, append_bytes.Length);

			return MemoryBuffer.ToArray();
		}
	}
}
