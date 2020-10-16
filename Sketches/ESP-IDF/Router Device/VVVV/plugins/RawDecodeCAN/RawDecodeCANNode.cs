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

namespace VVVV.Nodes
{
	#region PluginInfo
	[PluginInfo(Name = "DecodeCAN", Category = "Raw")]
	#endregion PluginInfo
	public class RawDecodeCANNode : IPluginEvaluate, IPartImportsSatisfiedNotification
	{
		#region fields & pins
		[Input("Input")]
		public ISpread<Stream> FInput;

		[Output("TargetID")]
		public ISpread<int> FOutTargetID;
		
		[Output("Operation")]
		public ISpread<int> FOutOperation;
		
		[Output("RegisterID")]
		public ISpread<int> FOutRegisterID;
		
		[Output("Value")]
		public ISpread<int> FOutValue;

		//when dealing with byte streams (what we call Raw in the GUI) it's always
		//good to have a byte buffer around. we'll use it when copying the data.
		#endregion fields & pins

		//called when all inputs and outputs defined above are assigned from the host
		public void OnImportsSatisfied()
		{
			//start with an empty stream output
		}

		//called when data for any output pin is requested
		public void Evaluate(int spreadMax)
		{
			FOutTargetID.SliceCount = spreadMax;
			FOutOperation.SliceCount = spreadMax;
			FOutRegisterID.SliceCount = spreadMax;
			FOutValue.SliceCount = spreadMax;
			
			for (int i = 0; i < spreadMax; i++) {
				var input = FInput[i];
				var buffer = new char[input.Length];
				for(int j=0; j<input.Length; j++) {
					buffer[j] = (char) input.ReadByte();
				}
				
				var rawData = System.Convert.FromBase64CharArray(buffer, 0, buffer.Length);
				FOutTargetID[i] = (int) rawData[0];
				FOutOperation[i] = (int) rawData[1];
				FOutRegisterID[i] = ((int) rawData[2]) + (((int) rawData[3]) << 8);
				FOutValue[i] = ((int) rawData[4])
							+ (((int) rawData[5]) << 8)
							+ (((int) rawData[6]) << 16)
							+ (((int) rawData[7]) << 24)
				;
			}
		}
	}
}
