using MuscleMemory;
using System.ComponentModel.Composition;
using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V2;

namespace VVVV.MuscleMemory
{
	#region PluginInfo
	[PluginInfo(Name = "Ping",
				Category = "MuscleMemory",
				Help = "Ping a motor",
				Tags = "CAN",
				AutoEvaluate = true
		)]
	#endregion PluginInfo
	public class PingNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Bus Group", IsSingle = true)]
		public ISpread<BusGroup> FInBusGroup;

		[Input("ID", DefaultValue = 1)]
		public ISpread<int> FInID;

		[Input("Blind")]
		public ISpread<bool> FInBlind;

		[Input("Do", IsBang = true)]
		public ISpread<bool> FInDo;

		[Output("Bus Group")]
		public ISpread<BusGroup> FOutBusGorup;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			FOutBusGorup[0] = FInBusGroup[0];

			if(FInBusGroup[0] == null)
			{
				return;
			}

			for(int i=0; i<SpreadMax; i++)
			{
				if(FInDo[i])
				{
					if (FInBlind[i])
					{
						FInBusGroup[0].PingBlind(FInID[i]);
					}
					else
					{
						var motor = FInBusGroup[0].FindMotor(FInID[i]);
						motor.Ping();
					}
				}
			}
		}
	}
}
