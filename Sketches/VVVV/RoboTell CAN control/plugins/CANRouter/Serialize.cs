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
	[PluginInfo(Name = "Serialize", Category = "CAN", Tags = "", AutoEvaluate=true)]
	#endregion PluginInfo
	public class SerializerCANNode : IPluginEvaluate, IPartImportsSatisfiedNotification
	{
		#region fields & pins
		[Input("Input")]
		public ISpread<CanMessage> FInput;

		[Output("Output")]
		public ISpread<Stream> FOutput;
		
		[Output("EncodedStrings")]
		public ISpread<string> FOutStrings;
		
		#endregion fields & pins

		//called when all inputs and outputs defined above are assigned from the host
		public void OnImportsSatisfied()
		{
			//start with an empty stream output
			FOutput.SliceCount = 1;
		}
		
		//called when data for any output pin is requested
		public void Evaluate(int spreadMax)
		{
			var outputStream = new MemoryStream();
			var outputWriter = new StreamWriter(outputStream);

			FOutStrings.SliceCount = 0;
			
			for (int i = 0; i < spreadMax; i++) {
				var message = FInput[i];
				if(message == null) {
					continue;
				}
				var binaryStream = new MemoryStream();
				var binaryWriter = new BinaryWriter(binaryStream);
				binaryWriter.Write((UInt32) message.Flags);
				binaryWriter.Write((UInt32) message.Identifier);
				binaryWriter.Write((byte) message.Length);
				for(int y=0; y<8; y++) {
					binaryWriter.Write(message.Data[y]);
				}
				
				var bytes = binaryStream.ToArray();
				var base64String = System.Convert.ToBase64String(bytes, 0, bytes.Length);
				FOutStrings.Add(base64String);
				
				outputWriter.Write(base64String);
				outputWriter.Write((byte) 0xFF);
			}
			
			FOutput[0] = outputStream;
		}
	}
}
