using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
namespace NewCommon.Network
{
	public class Shared
	{
		public static UInt16 PackageSignature = 0xAAE0;
		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		public struct PackageInformation
		{
			public uint PackageTotalSize;
			public ushort PackageMagic;
		};


		public static int GetPackageInformationLength()
		{
			return Marshal.SizeOf(typeof(PackageInformation));
		}
	}

	public class BadPackageException : Exception
	{
		public BadPackageException()
			: base()
		{
		}
	}
}
