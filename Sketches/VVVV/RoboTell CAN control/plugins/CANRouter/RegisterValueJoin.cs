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
	[PluginInfo(Name = "RegisterValue", Category = "CAN", Version = "Join", Tags = "")]
	#endregion PluginInfo
	public class JoinCANRegisterValueNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Flags")]
		public ISpread<int> FInFlags;

		[Input("ID", MinValue=0, MaxValue=1023)]
		public ISpread<int> FInID;

		[Input("Operation", MinValue=0, MaxValue=255)]
		public ISpread<int> FInOperation;

		[Input("RegisterType")]
		public ISpread<int> FInRegisterType;

		[Input("Value")]
		public ISpread<int> FInValue;
		
		[Output("Output")]
		public ISpread<CanMessage> FOutput;
		
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int spreadMax)
		{
			FOutput.SliceCount = 0;

			for (int i = 0; i < spreadMax; i++) {
				var message = new CanMessage();
				message.Flags = (UInt32) FInFlags[i];
				message.Identifier = (UInt32) FInID[i] << 19;
				message.Length = 7;
				
				message.Data[0] = (byte) (FInOperation[i]);
				
				message.Data[1] = (byte) (FInRegisterType[i] & 255);
				message.Data[2] = (byte) (FInRegisterType[i] >> 8);
				
				message.Data[3] = (byte) (FInValue[i] & 255);
				message.Data[4] = (byte) ((FInValue[i] >> 8) & 255);
				message.Data[5] = (byte) ((FInValue[i] >> 16) & 255);
				message.Data[6] = (byte) ((FInValue[i] >> 24) & 255);
				
				FOutput.Add(message);
			}
		}
	}
}
