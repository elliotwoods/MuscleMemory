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
	[PluginInfo(Name = "RegisterValue", Category = "CAN", Version = "Split", Tags = "")]
	#endregion PluginInfo
	public class SplitCANRegisterValueNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Input")]
		public ISpread<CanMessage> FInput;

		[Output("Flags")]
		public ISpread<int> FOutFlags;

		[Output("ID")]
		public ISpread<int> FOutID;

		[Output("Operation")]
		public ISpread<int> FOutOperation;

		[Output("RegisterType")]
		public ISpread<int> FOutRegisterType;

		[Output("Value")]
		public ISpread<int> FOutValue;
		
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int spreadMax)
		{
			FOutFlags.SliceCount = 0;
			FOutID.SliceCount = 0;
			FOutOperation.SliceCount = 0;
			FOutRegisterType.SliceCount = 0;
			FOutValue.SliceCount = 0;

			for (int i = 0; i < spreadMax; i++) {
				var message = FInput[i];
				if (message != null) {
					if(message.Length == 7) {
						var operation = message.Data[0];
						if(operation < 100) {
							int registerIndex = 0;
							registerIndex += (int) message.Data[2] << 8;
							registerIndex += (int) message.Data[1];
							
							int registerValue = 0;
							registerValue += (int) message.Data[3];
							registerValue += (int) message.Data[4] << 8;
							registerValue += (int) message.Data[5] << 16;
							registerValue += (int) message.Data[6] << 24;
							
							var ID = message.Identifier >> 19;
							
							FOutFlags.Add((int)message.Flags);
							FOutID.Add((int)ID);
							FOutOperation.Add((int) operation);
							FOutRegisterType.Add(registerIndex);
							FOutValue.Add(registerValue);
						}
					}
					
				}
			}
		}
	}
}
