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
	[PluginInfo(Name = "OTASend", Category = "Robotell", Version = "CAN", Help = "Basic raw template which copies up to count bytes from the input to the output", Tags = "")]
	#endregion PluginInfo
	public class CANRobotellOTASendNode : IPluginEvaluate, IPartImportsSatisfiedNotification
	{
		#region fields & pins
		[Input("ID", MinValue=0, MaxValue=1023)]
		public IDiffSpread<int> FIDIn;
		
		[Input("Input")]
        public ISpread<Stream> FDataIn;

		[Input("Speed", MinValue=0, DefaultValue=4)]
		public ISpread<int> FSpeedIn;
		
		[Input("Reset", IsBang=true)]
		public ISpread<bool> FResetIn;
		
		[Input("RequestPosition")]
		public ISpread<ISpread<int>> FRequestPositionIn;
		
		[Input("Send")]
		public ISpread<bool> FSendIn;

		[Output("Output")]
		public ISpread<Stream> FStreamOut;
		
		[Output("Position")]
		public ISpread<int> FPositionOut;
		
		[Output("Size")]
		public ISpread<int> FSizeOut;
		#endregion fields & pins
		
		class Status {
			public bool beginSent = false;
			public int position = 0;
		}
		
		List<Status> FStatus = new List<Status>();

		//called when all inputs and outputs defined above are assigned from the host
		public void OnImportsSatisfied()
		{
			//start with an empty stream output
			FStreamOut.SliceCount = 0;
		}
		
		long WriteHeader(Stream stream, int targetID)
		{
			var startPosition = stream.Position;
			
			// HEADER
			stream.WriteByte(0xAA);
			stream.WriteByte(0xAA);
			
			// TARGET ID
			var fullID = targetID;
			stream.WriteByte((byte) (fullID & 255));
			stream.WriteByte((byte) ((fullID >> 8)& 255));
			stream.WriteByte((byte) ((fullID >> 16)& 255));
			stream.WriteByte((byte) ((fullID >> 24)& 255));
			
			return startPosition;
		}
		
		void WriteFooter(Stream stream, long startPosition)
		{
			// CAN MESSAGE
			stream.WriteByte(0x00);
			
			// MESSAGE TYPE (0=Standard, 1=Extended)
			stream.WriteByte(0x01);
			
			// REQUEST TYPE (0=Data frame, 1=Remote request)
			stream.WriteByte(0x00);
			
			// CALCULATE CRC
			{
				stream.Seek(startPosition + 2, SeekOrigin.Begin);
				int crc = 0;
				for(int byteIndex = 0; byteIndex < 16; byteIndex++) {
					var value = stream.ReadByte();
					crc += value;
				}
				
				// ADD CRC
				crc = crc & 0xFF;
				switch(crc) {
					case 0xA5:
					case 0xAA:
					case 0x55:
						stream.WriteByte(0xA5);
						break;
					default:
						break;
				}
				stream.WriteByte((byte) crc);
			}
			
			// END MARK
			stream.WriteByte(0x55);
			stream.WriteByte(0x55);
		}
		
		void WriteOTABegin(Stream stream, int targetID, UInt32 size)
		{
			var startPosition = WriteHeader(stream, targetID);
			
			// OPERATION
			stream.WriteByte((byte) 100);
		
			var binaryStream = new BinaryWriter(stream);
		
			// CONTENT
			binaryStream.Write((UInt32) size);
			
			// DUMMY BYTES
			stream.WriteByte(0x00);
			stream.WriteByte(0x00);
			stream.WriteByte(0x00);
		
			// LENGTH
			stream.WriteByte(4);
			
			this.WriteFooter(stream, startPosition);
		}
		
		int WriteOTAData(Stream output, int targetID, Stream input, int position)
		{
			var startPosition = WriteHeader(output, targetID);
			
			// OPERATION
			output.WriteByte((byte) 101);
		
			var binaryStream = new BinaryWriter(output);
		
			// POSITION
			output.WriteByte((byte) (position & 255));
			output.WriteByte((byte) ((position >> 8)& 255));
			output.WriteByte((byte) ((position >> 16)& 255));
			
			// DATA
			output.WriteByte((byte) input.ReadByte());
			output.WriteByte((byte) input.ReadByte());
			output.WriteByte((byte) input.ReadByte());
			output.WriteByte((byte) input.ReadByte());
		
			// LENGTH
			output.WriteByte(8);
			
			this.WriteFooter(output, targetID);
			
			return position + 4;
		}
		

		//called when data for any output pin is requested
		public void Evaluate(int spreadMax)
		{
			//ResizeAndDispose will adjust the spread length and thereby call
			//the given constructor function for new slices and Dispose on old
			//slices.
			spreadMax = FDataIn.SliceCount;
			
			FStreamOut.ResizeAndDispose(spreadMax, () => new MemoryStream());
			FPositionOut.SliceCount = spreadMax;
			
			while(this.FStatus.Count < spreadMax) {
				this.FStatus.Add(new Status());
			}
			for (int i = 0; i < spreadMax; i++) {
				var outputStream = FStreamOut[i];
				if(this.FResetIn[i]) {
					this.FStatus[i] = new Status();
				}
				var status = this.FStatus[i];
				
				outputStream.SetLength(0);
				
				if(FRequestPositionIn[i].SliceCount > 0) {
					foreach(var requestPosition in FRequestPositionIn[i]) {
						if(requestPosition < status.position) {
							status.position = requestPosition;
						}
					}
				}
				
				if(FSendIn[i]) {
					outputStream.Seek(0, SeekOrigin.Begin);
					var dataLength = FDataIn[i].Length;
					if(!status.beginSent) {
						this.WriteOTABegin(outputStream, FIDIn[i], (UInt32) dataLength);
						status.beginSent = true;
					}
					else {
						if(status.position < dataLength) {
							for(int c=0; c<FSpeedIn[i]; c++) {
								status.position = this.WriteOTAData(outputStream, FIDIn[i], FDataIn[i], status.position);
								if(status.position >= FDataIn[i].Length) {
									break;
								}
							}
						}
					}
					
					FPositionOut[i] = status.position;
				}
				
				FSizeOut[i] = (int) FDataIn[i].Length;
			}
			//this will force the changed flag of the output pin to be set
			FStreamOut.Flush(true);
		}
	}
}
