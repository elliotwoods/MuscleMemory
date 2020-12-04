using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace GCAN
{

	public static class NativeFunctions
	{
		[Flags]
		public enum ECANStatus : uint
		{
			/// <summary>
			///  error
			/// </summary>
			STATUS_ERR = 0x00000,
			/// <summary>
			/// No error
			/// </summary>
			STATUS_OK = 0x00001,
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct CAN_OBJ
		{
			public uint ID;
			public uint TimeStamp;
			public byte TimeFlag;
			public byte SendType;
			public byte RemoteFlag;
			public byte ExternFlag;
			public byte DataLen;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
			public byte[] data;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
			public byte[] Reserved;
		}

		public struct CAN_ERR_INFO
		{
			public uint ErrCode;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
			public byte[] Passive_ErrData;
			public byte ArLost_ErrData;
		}

		public struct INIT_CONFIG
		{

			public uint AccCode;
			public uint AccMask;
			public uint Reserved;
			public byte Filter;
			public byte Timing0;
			public byte Timing1;
			public byte Mode;
		}

		public struct BOARD_INFO
		{
			public ushort hw_Version;
			public ushort fw_Version;
			public ushort dr_Version;
			public ushort in_Version;
			public ushort irq_Num;
			public byte can_Num;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 20)]
			public byte[] str_Serial_Num;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 40)]
			public byte[] str_hw_Type;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
			public ushort[] Reserved;
		}


		[DllImport("ECANVCI64.dll", EntryPoint = "OpenDevice")]
		public static extern ECANStatus OpenDevice(
			UInt32 DeviceType,
			UInt32 DeviceInd,
			UInt32 Reserved);


		[DllImport("ECANVCI64.dll", EntryPoint = "CloseDevice")]
		public static extern ECANStatus CloseDevice(
			UInt32 DeviceType,
			UInt32 DeviceInd);


		[DllImport("ECANVCI64.dll", EntryPoint = "InitCAN")]
		public static extern ECANStatus InitCAN(
			UInt32 DeviceType,
			UInt32 DeviceInd,
			UInt32 CANInd,
			ref INIT_CONFIG InitConfig);


		[DllImport("ECANVCI64.dll", EntryPoint = "StartCAN")]
		public static extern ECANStatus StartCAN(
			UInt32 DeviceType,
			UInt32 DeviceInd,
			UInt32 CANInd);


		[DllImport("ECANVCI64.dll", EntryPoint = "ResetCAN")]
		public static extern ECANStatus ResetCAN(
			UInt32 DeviceType,
			UInt32 DeviceInd,
			UInt32 CANInd);


		[DllImport("ECANVCI64.dll", EntryPoint = "Transmit")]
		public static extern ECANStatus Transmit(
			UInt32 DeviceType,
			UInt32 DeviceInd,
			UInt32 CANInd,
			CAN_OBJ[] Send,
			UInt16 length);


		[DllImport("ECANVCI64.dll", EntryPoint = "Receive")]
		public static extern ECANStatus Receive(
			UInt32 DeviceType,
			UInt32 DeviceInd,
			UInt32 CANInd,
			out CAN_OBJ Receive,
			UInt32 length,
			UInt32 WaitTime);


		[DllImport("ECANVCI64.dll", EntryPoint = "ReadErrInfo")]
		public static extern ECANStatus ReadErrInfo(
			UInt32 DeviceType,
			UInt32 DeviceInd,
			UInt32 CANInd,
			out CAN_ERR_INFO ReadErrInfo);


		[DllImport("ECANVCI64.dll", EntryPoint = "ReadBoardInfo")]
		public static extern ECANStatus ReadBoardInfo(
			UInt32 DeviceType,
			UInt32 DeviceInd,
			out BOARD_INFO ReadErrInfo);

		public static void ThrowIfError(ECANStatus status, UInt32 deviceType, UInt32 deviceIndex, UInt32 channel)
		{
			if(status != ECANStatus.STATUS_OK)
			{
				CAN_ERR_INFO errorInfo;
				if(ReadErrInfo(deviceType, deviceIndex, channel, out errorInfo) == ECANStatus.STATUS_OK)
				{
					switch(errorInfo.ErrCode)
					{
						case 0x0100:
							throw (new Exception("Device already open"));
						case 0x0200:
							throw (new Exception("Open device error"));
						case 0x0400:
							throw (new Exception("Device did not open"));
						case 0x0800:
							throw (new Exception("Buffer overflow"));
						case 0x1000:
							throw (new Exception("Device not exist"));
						case 0x2000:
							throw (new Exception("Load dll failure"));
						case 0x4000:
							throw (new Exception("Execute the command failure error"));
						case 0x8000:
							throw (new Exception("Insufficient memory"));
						case 0x0001:
							throw (new Exception("CAN controller FIFO overflow"));
						case 0x0002:
							throw (new Exception("CAN controller error alarm"));
						case 0x0004:
							throw (new Exception("CAN controller passive error"));
						case 0x0008:
							throw (new Exception("CAN controller arbitration lose"));
						case 0x0010:
							throw (new Exception("CAN controller bus error"));
						case 0x0020:
							throw (new Exception("CAN receive register full"));
						case 0x0040:
							throw (new Exception("CAN receiv register overflow"));
						case 0x0080:
							throw (new Exception("CAN controller active error"));
						default:
							throw (new Exception("Unknown GCAN error"));
					}
				}
			}
		}
	}
}
