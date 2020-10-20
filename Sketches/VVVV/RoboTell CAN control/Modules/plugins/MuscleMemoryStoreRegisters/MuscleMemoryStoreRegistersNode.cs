#region usings
using System;
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
	[PluginInfo(Name = "StoreRegisters", Category = "MuscleMemory", Help = "Basic template with one value in/out", Tags = "")]
	#endregion PluginInfo
	public class MuscleMemoryStoreRegistersNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Register ID")]
		public ISpread<int> FInRegisterID;
		
		[Input("Value")]
		public ISpread<int> FInValue;

		[Input("Register Selection")]
		public ISpread<int> FInRegisterSelection;

		[Output("Output")]
		public ISpread<int> FOutput;
		
		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			FOutput.SliceCount = FInRegisterSelection.SliceCount;
			
			SpreadMax = Math.Max(FInRegisterID.SliceCount, FInValue.SliceCount);
			if(FInRegisterID.SliceCount == 0 || FInValue.SliceCount == 0) {
				SpreadMax = 0;
			}
			
			var registerSelection = new List<int>(FInRegisterSelection);
			for(int i=0; i<SpreadMax; i++) {
				var find = registerSelection.IndexOf(FInRegisterID[i]);
				if(find == -1) {
					continue;
				}
				FOutput[find] = FInValue[i];
			}
		}
	}
}
