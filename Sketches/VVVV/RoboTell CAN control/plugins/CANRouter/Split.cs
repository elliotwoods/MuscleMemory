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
	[PluginInfo(Name = "CanMessage", Category = "CAN", Version="Split", Tags = "")]
	#endregion PluginInfo
	public class SplitCANCanMessageNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Input")]
		public ISpread<CanMessage> FInput;

		[Output("Flags")]
		public ISpread<int> FOutFlags;
	
		[Output("Identifier")]
		public ISpread<int> FOutIdentifier;
		
		[Output("Length")]
		public ISpread<int> FOutLength;

		[Output("Data")]
		public ISpread<Stream> FOutData;
		
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int spreadMax)
		{
			FOutFlags.SliceCount = 0;
			FOutIdentifier.SliceCount = 0;
			FOutLength.SliceCount = 0;
			FOutData.SliceCount = 0;
			
			for (int i = 0; i < spreadMax; i++) {
				var message = FInput[i];
				if(message != null) {
					FOutFlags.Add((int) message.Flags);
					FOutIdentifier.Add((int) message.Identifier);
					FOutLength.Add(message.Length);
					FOutData.Add(new MemoryStream(message.Data));
				}
			}
        }
	}
}
