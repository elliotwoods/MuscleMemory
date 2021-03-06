#region usings
using System;
using System.IO;
using System.ComponentModel.Composition;

using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;
using VVVV.Utils.VColor;
using VVVV.Utils.VMath;

using VVVV.Core.Logging;
#endregion usings

namespace VVVV.Nodes.RoboTell
{
	#region PluginInfo
	#endregion PluginInfo
	public class JoinRobotellCanMessageNode : IPluginEvaluate, IPartImportsSatisfiedNotification
	{
		#region fields & pins
		[Input("ID", MinValue = 0, MaxValue = 1023)]
		public IDiffSpread<int> FIDIn;

		[Input("Operation")]
		public IDiffSpread<Operation> FInOperation;

		[Input("Register ID", MinValue = 0, MaxValue = 255)]
		public IDiffSpread<int> FInRegisterID;

		[Input("Value")]
		public IDiffSpread<int> FInValue;

		[Output("Output")]
		public ISpread<Stream> FStreamOut;

		//when dealing with byte streams (what we call Raw in the GUI) it's always
		//good to have a byte buffer around. we'll use it when copying the data.
		readonly byte[] FBuffer = new byte[1024];
		#endregion fields & pins

		//called when all inputs and outputs defined above are assigned from the host
		public void OnImportsSatisfied()
		{
			//start with an empty stream output
			FStreamOut.SliceCount = 0;
		}

		//called when data for any output pin is requested
		public void Evaluate(int spreadMax)
		{
			//ResizeAndDispose will adjust the spread length and thereby call
			//the given constructor function for new slices and Dispose on old
			//slices.
			FStreamOut.ResizeAndDispose(spreadMax, () => new MemoryStream());

			for (int i = 0; i < spreadMax; i++) {
				//get the output stream (this works because of ResizeAndDispose above)
				if (FIDIn.IsChanged || FInOperation.IsChanged || FInRegisterID.IsChanged || FInValue.IsChanged) {
					var outputStream = FStreamOut[i];
					outputStream.Seek(0, SeekOrigin.Begin);

					// HEADER
					outputStream.WriteByte(0xaa);
					outputStream.WriteByte(0xaa);

					// TARGET ID
					var fullID = FIDIn[i] << (29 - 10);
					outputStream.WriteByte((byte)(fullID & 255));
					outputStream.WriteByte((byte)((fullID >> 8) & 255));
					outputStream.WriteByte((byte)((fullID >> 16) & 255));
					outputStream.WriteByte((byte)((fullID >> 24) & 255));

					// OPERATION
					switch (FInOperation[i]) {
						case Operation.Read:
							outputStream.WriteByte(0x0);
							break;
						case Operation.Write:
							outputStream.WriteByte(0x1);
							break;
						case Operation.WriteDefault:
							outputStream.WriteByte(0x1);
							break;
					}

					var binaryStream = new BinaryWriter(outputStream);

					// REGISTER ID
					binaryStream.Write((UInt16)(FInRegisterID[i]));

					// VALUE
					binaryStream.Write(FInValue[i]);

					// DUMMY BYTES
					outputStream.WriteByte(0x0);

					// LENGTH
					outputStream.WriteByte(7);

					// CAN MESSAGE
					outputStream.WriteByte(0x0);

					// MESSAGE TYPE (0=Standard, 1=Extended)
					outputStream.WriteByte(0x1);

					// REQUEST TYPE (0=Data frame, 1=Remote request)
					outputStream.WriteByte(0x0);

					// CALCULATE CRC
					{
						outputStream.Seek(2, SeekOrigin.Begin);
						bool needsFrameCtrl = false;
						int crc = 0;
						for (int byteIndex = 0; byteIndex < 16; byteIndex++) {
							var value = outputStream.ReadByte();
							switch (value) {
								case 0xa5:
								case 0xaa:
								case 0x55:
									needsFrameCtrl = true;
									break;
								default:
									break;
							}
							crc += value;
						}

						// ADD CRC
						crc = crc & 0xff;
						outputStream.WriteByte((byte)crc);


						// ADD FrameCtrl IF NEEDED
						if (needsFrameCtrl) {
							outputStream.WriteByte(0xa5);
						}
					}

					// END MARK
					outputStream.WriteByte(0x55);
					outputStream.WriteByte(0x55);
				}
			}
			//this will force the changed flag of the output pin to be set
			FStreamOut.Flush(true);
		}
	}
}
