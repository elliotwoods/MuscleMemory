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

namespace VVVV.Nodes
{
	#region PluginInfo
	[PluginInfo(Name = "Deserialize", Category = "CAN", Tags = "", AutoEvaluate=true)]
	#endregion PluginInfo
	public class DeserializerCANNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Input")]
		public ISpread<Stream> FInput;

		[Output("Output")]
		public ISpread<CanMessage> FOutput;
	
		[Output("Errors")]
		public ISpread<int> FOutErrors;
		
		[Output("Buffer Length")]
		public ISpread<int> FOutBufferLength;
		
		List<char> FIncomingBuffer = new List<char>();
		int FErrors = 0;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int spreadMax)
		{
			FOutput.SliceCount = 0;
			
			for (int i = 0; i < spreadMax; i++) {
				//get the input stream
				var inputStream = FInput[i];
				var value = inputStream.ReadByte();
				while(value != -1) {
					if(value == 0xFF) {
						try {
							var message = this.DecodeBuffer();
							if(message != null) {
								FOutput.Add(message);
							}	
						}
						catch {
							this.FErrors++;
						}
						finally {
							this.FIncomingBuffer.Clear();
						}
						
					}
					else {
						if(value != 0x0A) {
							this.FIncomingBuffer.Add((char) value);
						}
					}
					value = inputStream.ReadByte();
				}
				
			}
			FOutBufferLength[0] = FIncomingBuffer.Count;
			FOutErrors[0] = this.FErrors;
			//this will force the changed flag of the output pin to be set
		}
		
		CanMessage DecodeBuffer()
		{
			var bytes = System.Convert.FromBase64CharArray(this.FIncomingBuffer.ToArray(), 0, this.FIncomingBuffer.Count);
			if(bytes.Length > 4 + 4 + 1 + 8) {
				var message = new CanMessage();
				using (MemoryStream stream = new MemoryStream(bytes)) {
	            	using (BinaryReader binaryReader = new BinaryReader(stream)) {
	            		message.Flags = binaryReader.ReadUInt32();
						message.Identifier = binaryReader.ReadUInt32();
						message.Length = (byte) binaryReader.Read();
	            		for(int i=0; i<message.Length; i++) {
	            			message.Data[i] = binaryReader.ReadByte();
	            		}
	            	}
				}
				return message;
			}
			else {
				return null;
			}
        }
	}
}
