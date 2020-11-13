#region usings
using System;
using System.IO;
using System.ComponentModel.Composition;
using System.Collections.Generic;

using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;
using VVVV.Utils.VColor;
using VVVV.Utils.VMath;

using VVVV.Core.Logging;
#endregion usings

namespace VVVV.Nodes.RoboTell
{
	#region PluginInfo
	[PluginInfo(Name = "OTAResponse", Category = "Robotell", Tags = "")]
	#endregion PluginInfo
	public class RobotellOTAResponseNode : IPluginEvaluate, IPartImportsSatisfiedNotification
	{
		#region fields & pins
		[Input("Input")]
		public ISpread<Stream> FStreamIn;

		[Output("ID")]
		public ISpread<int> FOutID;

		[Output("Request Info")]
		public ISpread<bool> FOutRequestInfo;

		[Output("Request Data")]
		public ISpread<bool> FOutRequestData;

		[Output("Position")]
		public ISpread<int> FOutPosition;

		//when dealing with byte streams (what we call Raw in the GUI) it's always
		//good to have a byte buffer around. we'll use it when copying the data.
		int FReadStatus = 0;
		List<byte> FMessageBuffer = new List<byte>();
		List<List<byte>> FCompleteMessages = new List<List<byte>>();

		int FCRC = 0;
		#endregion fields & pins

		//called when all inputs and outputs defined above are assigned from the host
		public void OnImportsSatisfied()
		{

		}

		public void processByte(byte value)
		{
			switch (this.FReadStatus) {
				case 0:
					
					{
						if (value == 0xaa) {
							this.FReadStatus = 1;
							this.FMessageBuffer.Clear();
							this.FMessageBuffer.Add(value);
							this.FCRC = 0;
						}
					}


					break;
				case 1:
					if (value == 0xaa) {
						this.FReadStatus = 2;
						this.FMessageBuffer.Add(value);
					} else {
						this.FMessageBuffer.Clear();
						this.FReadStatus = 0;
						throw (new Exception("Invalid packet (1)"));
					}
					break;
				case 2:
					
					{
						if (FMessageBuffer.Count == 18) {
							// FrameCtl, next is CRC
							if (value == 0xa5) {
								this.FReadStatus = 3;
							} else {
								this.FCRC = this.FCRC & 0xff;
								if (this.FCRC != value) {
									this.FReadStatus = 0;
									throw (new Exception("CRC check failed (2)"));
								} else {
									this.FReadStatus = 8;
									this.FMessageBuffer.Add(value);
								}
							}
						} else {
							if (value == 0xa5) {
								// This is a FrameCtl char - ignore it. But how does that not break?
								this.FReadStatus = 4;
							} else {
								this.FMessageBuffer.Add(value);
								this.FCRC += value;
							}
						}
					}

					break;
				case 3:
					
					{
						// CRC after FrameCtl
						this.FCRC = this.FCRC & 0xff;
						if (this.FCRC != value) {
							this.FReadStatus = 0;
							throw (new Exception("CRC check failed (3)"));
						} else {
							this.FReadStatus = 8;
							this.FMessageBuffer.Add(value);
						}
					}

					break;
				case 4:
					
					{
						// Reading after FrameCtl
						this.FCRC += value;
						this.FMessageBuffer.Add(value);
						this.FReadStatus = 2;
					}

					break;
				case 8:
					
					{
						if (value == 0x55) {
							this.FReadStatus = 9;
							this.FMessageBuffer.Add(value);
						} else {
							this.FReadStatus = 0;
							throw (new Exception("Invalid packet (8)"));
						}
					}

					break;
				case 9:
					
					{
						if (value == 0x55) {
							this.FMessageBuffer.Add(value);
							this.FReadStatus = 9;
							if (this.FMessageBuffer.Count == 21) {
								FCompleteMessages.Add(this.FMessageBuffer);
								this.FMessageBuffer = new List<byte>();
								this.FReadStatus = 0;
							} else {
								throw (new Exception("Invalid packet (9A)"));
							}
						} else {
							this.FReadStatus = 0;
							throw (new Exception("Invalid packet (9B)"));
						}
					}

					break;
			}
		}

		//called when data for any output pin is requested
		public void Evaluate(int spreadMax)
		{
			// Add incoming to buffer
			foreach (var inputSlice in FStreamIn) {
				int readResult = inputSlice.ReadByte();
				while (readResult != -1) {
					this.processByte((byte)readResult);
					//FRingBuffer.Put((byte) readResult);
					readResult = inputSlice.ReadByte();
				}

				FOutID.SliceCount = 0;
				FOutRequestInfo.SliceCount = 0;
				FOutRequestData.SliceCount = 0;
				FOutPosition.SliceCount = 0;

				foreach (var buffer in this.FCompleteMessages) {
					int ID;
					int operation;

					if (buffer[15] == 0xff) {
						// System message, ignore it
						continue;
					}

					if (buffer[16] == 0) {
						// Standard
						ID = (buffer[3] << 8) + buffer[2];
					} else {
						// Extended
						ID = (buffer[5] << 24) + (buffer[4] << 16) + (buffer[3] << 8) + buffer[2];
					}

					var binaryReader = new BinaryReader(new MemoryStream(buffer.ToArray()));

					// HEADER
					binaryReader.Read();
					binaryReader.Read();

					// FRAME ID
					binaryReader.Read();
					binaryReader.Read();
					binaryReader.Read();
					binaryReader.Read();

					// OPERATION
					operation = binaryReader.Read();
					
					if(operation == 102) {
						FOutID.Add(ID);
						FOutRequestInfo.Add(true);
						FOutRequestData.Add(false);
						FOutPosition.Add(0);
					}
					else if(operation == 103) {
						var position = binaryReader.ReadUInt32();
						FOutID.Add(ID);
						FOutRequestInfo.Add(false);
						FOutRequestData.Add(true);
						FOutPosition.Add((int) position);
					}
				}
				this.FCompleteMessages.Clear();
			}
		}
	}
}
