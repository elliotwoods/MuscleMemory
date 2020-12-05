using Candle;
using System;
using System.IO;

namespace MuscleMemory
{
	public class Messages
	{
		public const int MaxIndex = 1023;

		public enum Operation : byte
		{
			ReadRequest = 0,
			WriteRequest = 1,
			ReadResponse = 2,
			WriteAndSaveDefaultRequest = 3,

			Ping = 8,
			PingResponse = 9,

			OTARequests = 100, // mark the start of OTA requests in the enum
			OTAInfo = 100,
			OTAData = 101,
			OTARequestInfo = 102,
			OTARequestData = 103,
		}

		public enum RegisterType : UInt16
		{
			// Device
			DeviceID = 0,
			ControlMode = 1,

			// Position, Velocity, Target, Torque
			MultiTurnPosition = 10,
			Velocity = 11,
			TargetPosition = 12,
			Torque = 13,
			MaximumTorque = 14,
			SoftLimitMin = 15,
			SoftLimitMax = 16,
			TargetPositionFiltered = 17,
			MaxVelocity = 18,

			// Encoder
			EncoderReading = 21,
			EncoderErrors = 22,
			EncoderPositionFilterSize = 23,
			MultiTurnSaveEnabled = 27,
			ZeroPosSet = 28,
			ZeroPos = 29,

			// Power
			Current = 30,
			BusVoltage = 32,
			// ShuntVoltage = 33,

			// System
			FreeMemory = 40,
			CPUTemperature = 41,
			UpTime = 42,
			InterfaceEnabled = 43,

			// Update frequencies
			MotorControlFrequency = 50,
			AgentControlFrequency = 51,
			RegistryControlFrequency = 52,
			MainLoopDelay = 53,

			// Agent
			AgentLocalHistorySize = 60,
			AgentTraining = 61,
			AgentNoiseAmplitude = 62,
			AgentAddProportional = 63,
			AgentAddConstant = 64,

			// PID controller
			PIDProportional = 70,
			PIDIntegral = 71,
			PIDDifferential = 72,
			PIDIntegralMax = 73,
			PIDResultP = 74,
			PIDResultI = 75,
			PIDResultD = 76,

			// Offest control
			DriveOffset = 80,
			OffsetFactor = 81,
			OffsetMaximum = 82,

			// Anti-stall
			AntiStallEnabled = 90,
			AntiStallDeadZone = 91,
			AntiStallMinVelocity = 92,
			AntiStallAttack = 93,
			AntiStallDecay = 94,
			AntiStallValue = 95,
			AntiStallScale = 96,

			// CAN debug
			CANRxThisFrame = 150,
			CANTxThisFrame = 151,
			CANErrorsThisFrame = 152,
			CANErrorsTotal = 153,

			// Boot
			Reboot = 200,
			ProvisioningEnabled = 201,
			FastBoot = 202,
			CANWatchdogEnabled = 203,
			CANWatchdogTimeout = 204,
			CANWatchdogTimer = 205,

			// OTA
			OTADownloading = 210,
			OTAWritePosition = 211,
			OTASize = 212,
		}

		public interface IMessage
		{
			Frame Encode();
		}

		public class GenericRequest : IMessage
		{
			protected Operation FOperation;
			public int ID;
			public RegisterType RegisterType;
			public Int32 Value;

			public GenericRequest()
			{
				// This constructor is used when decoding
			}

			public GenericRequest(int ID, Operation operation, RegisterType registerType, Int32 value)
			{
				this.ID = ID;
				this.FOperation = operation;
				this.RegisterType = registerType;
				this.Value = value;
			}

			public void Decode(Frame frame)
			{
				this.ID = (int) (frame.Identifier >> 19);

				if (frame.Data.Length < 1 + 2 + 4)
				{
					throw (new Exception("Invalid message format"));
				}

				using (var memoryStream = new MemoryStream(frame.Data))
				{
					using (var binaryReader = new BinaryReader(memoryStream))
					{
						// Ignore operation
						binaryReader.ReadByte();

						this.RegisterType = (RegisterType)binaryReader.ReadUInt16();
						this.Value = binaryReader.ReadInt32();
					}
				}
			}

			public Frame Encode()
			{
				var frame = new Frame();
				frame.Identifier = (UInt32)(this.ID << 19);
				frame.Extended = true;
				frame.Data = new byte[1 + 2 + 4];
				using (var memoryStream = new MemoryStream(frame.Data))
				{
					using (var binaryWriter = new BinaryWriter(memoryStream))
					{
						binaryWriter.Write((byte) this.FOperation);
						binaryWriter.Write((UInt16)this.RegisterType);
						binaryWriter.Write((Int32)this.Value);
					}
				}
				return frame;
			}

			public override String ToString() 
			{
				return String.Format("[{0}] ID : {1}, RegisterType : {2}, Value : {3}"
					, this.FOperation.ToString()
					, this.ID
					, this.RegisterType.ToString()
					, this.Value);
			}
		}

		public class ReadRequest : IMessage
		{
			public int ID;
			public RegisterType RegisterType;

			public ReadRequest(int ID, RegisterType registerType)
			{
				this.ID = ID;
				this.RegisterType = registerType;
			}

			public ReadRequest(Frame frame)
			{
				this.ID = (int) (frame.Identifier >> 19);

				if (frame.Data.Length < 1 + 2)
				{
					throw (new Exception("Invalid message format"));
				}

				using (var memoryStream = new MemoryStream(frame.Data))
				{
					using (var binaryReader = new BinaryReader(memoryStream))
					{
						// Ignore operation
						binaryReader.ReadByte();
						this.RegisterType = (RegisterType)binaryReader.ReadUInt16();
					}
				}
			}

			public Frame Encode()
			{
				var frame = new Frame();
				frame.Identifier = (UInt32)(this.ID << 19);
				frame.Extended = true;
				frame.Data = new byte[1 + 2];
				using (var memoryStream = new MemoryStream(frame.Data))
				{
					using (var binaryWriter = new BinaryWriter(memoryStream))
					{
						binaryWriter.Write((byte)Operation.ReadRequest);
						binaryWriter.Write((UInt16)this.RegisterType);
					}
				}
				return frame;
			}

			public override String ToString()
			{
				return String.Format("[ReadRequest] ID : {0}, RegisterType : {1}"
					, this.ID
					, this.RegisterType.ToString());
			}
		}

		public class WriteRequest : GenericRequest
		{
			public WriteRequest(int ID, RegisterType registerType, Int32 value) : base(ID, Operation.WriteRequest, registerType, value)
			{
			}

			public WriteRequest(Frame frame)
			{
				this.FOperation = Operation.WriteRequest;
				this.Decode(frame);
			}
		}

		public class ReadResponse : GenericRequest
		{
			public ReadResponse(int ID, RegisterType registerType, Int32 value) : base(ID, Operation.ReadResponse, registerType, value)
			{
			}

			public ReadResponse(Frame frame)
			{
				this.FOperation = Operation.ReadResponse;
				this.Decode(frame);
			}
		}

		public class WriteAndSaveDefaultRequest : GenericRequest
		{
			public WriteAndSaveDefaultRequest(int ID, RegisterType registerType, Int32 value) : base(ID, Operation.WriteAndSaveDefaultRequest, registerType, value)
			{
			}

			public WriteAndSaveDefaultRequest(Frame frame)
			{
				this.FOperation = Messages.Operation.WriteAndSaveDefaultRequest;
				this.Decode(frame);
			}
		}

		public class WritePrimaryRegisterRequest : IMessage
		{
			public int ID;
			public Int32 Value;

			public WritePrimaryRegisterRequest(int ID, Int32 value)
			{
				this.ID = ID;
				this.Value = value;
			}

			public WritePrimaryRegisterRequest(Frame frame)
			{
				this.ID = (int)(frame.Identifier >> 1);

				if (frame.Data.Length < 4)
				{
					throw (new Exception("Invalid message format"));
				}

				using (var memoryStream = new MemoryStream(frame.Data))
				{
					using (var binaryReader = new BinaryReader(memoryStream))
					{
						this.Value = binaryReader.ReadInt32();
					}
				}
			}

			public Frame Encode()
			{
				var frame = new Frame();
				frame.Identifier = (UInt32)(this.ID << 1); // This is a standard frame. The mask works differently
				frame.Extended = false;
				frame.Data = new byte[4];
				using (var memoryStream = new MemoryStream(frame.Data))
				{
					using (var binaryWriter = new BinaryWriter(memoryStream))
					{
						binaryWriter.Write((Int32) this.Value);
					}
				}
				return frame;
			}
		}

		public class Ping : IMessage
		{
			public int ID;

			public Ping(int ID)
			{
				this.ID = ID;
			}

			public Ping(Frame frame)
			{
				this.ID = (int)(frame.Identifier >> 19);
			}

			public Frame Encode()
			{
				var frame = new Frame();
				frame.Identifier = (UInt32)(this.ID << 19);
				frame.Extended = true;
				frame.Data = new byte[1];
				frame.Data[0] = (byte)Operation.Ping;
				return frame;
			}
		}

		public class PingResponse : IMessage
		{
			public int ID;

			public PingResponse(int ID)
			{
				this.ID = ID;
			}

			public PingResponse(Frame frame)
			{
				this.ID = (int)(frame.Identifier >> 19);
			}

			public Frame Encode()
			{
				var frame = new Frame();
				frame.Identifier = (UInt32)(this.ID << 19);
				frame.Extended = true;
				frame.Data = new byte[1];
				frame.Data[0] = (byte)Operation.PingResponse;
				return frame;
			}
		}

		public static IMessage Decode(Frame frame)
		{
			if(frame.Data.Length < 1)
			{
				return null;
			}

			if(!frame.Extended)
			{
				return new WritePrimaryRegisterRequest(frame);
			}

			var operation = (Operation) frame.Data[0];
			switch (operation)
			{
				case Operation.ReadRequest:
					return new ReadRequest(frame);
				case Operation.WriteRequest:
					return new WriteRequest(frame);
				case Operation.ReadResponse:
					return new ReadResponse(frame);
				case Operation.WriteAndSaveDefaultRequest:
					return new WriteAndSaveDefaultRequest(frame);
				case Operation.PingResponse:
					return new PingResponse(frame);
				case Operation.OTARequests:
				case Operation.OTAData:
				case Operation.OTARequestInfo:
				case Operation.OTARequestData:
				default:
					return null;
			}
		}
	}
}
