using MuscleMemory;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V2;

namespace VVVV.MuscleMemory
{
	#region PluginInfo
	[PluginInfo(Name = "MotorInfo",
				Category = "MuscleMemory",
				Help = "Get info for Motor",
				Tags = "CAN")]
	#endregion PluginInfo
	public class MotorInfoNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Input")]
		public ISpread<Motor> FInput;

		[Output("ID")]
		public ISpread<int> FOutID;

		[Output("Last Seen")]
		public ISpread<string> FOutLastSeen;

		[Output("Bus")]
		public ISpread<Bus> FOutBus;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			FOutID.SliceCount = 0;
			FOutLastSeen.SliceCount = 0;
			FOutBus.SliceCount = 0;

			foreach(var motor in FInput)
			{
				if(motor != null)
				{
					FOutID.Add(motor.ID);
					FOutLastSeen.Add(motor.LastSeen.ToString());
					FOutBus.Add(motor.Bus);
				}
			}
		}
	}
}
