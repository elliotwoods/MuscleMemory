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
	[PluginInfo(Name = "ManualAddMotor",
				Category = "MuscleMemory",
				Help = "Manually add a motor to a Bus",
				Tags = "CAN",
				AutoEvaluate = true
		)]
	#endregion PluginInfo
	public class ManualAddMotorNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Bus", IsSingle = true)]
		public ISpread<Bus> FInBus;

		[Input("Motor ID", DefaultValue = 1)]
		public ISpread<ISpread<int>> FInID;

		[Input("Add", IsBang = true)]
		public ISpread<ISpread<bool>> FInAdd;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			for (int iBus = 0; iBus < FInBus.SliceCount; iBus++)
			{
				var bus = FInBus[iBus];
				if (bus != null)
				{
					for (int iMotorID = 0; iMotorID < FInID[iBus].SliceCount; iMotorID++)
					{
						bus.NotifyMotorExists(FInID[iBus][iMotorID]);
					}
				}
			}
		}
	}
}
