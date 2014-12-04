using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
namespace NewCommon.Data
{
	public class BufferEx
	{
		public static MemoryStream MemoryStreamToReaderStream(MemoryStream memoryStream)
		{
			return new MemoryStream(memoryStream.GetBuffer(), 0, (int)memoryStream.Length);
		}
		public static byte[] MemoryStreamToBytes(MemoryStream memoryStream)
		{
			return new BinaryReader(MemoryStreamToReaderStream(memoryStream)).ReadBytes((int)memoryStream.Length);
		}
	}
}
