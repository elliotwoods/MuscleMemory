using MuscleMemory;
using System.ComponentModel.Composition;
using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V2;

namespace VVVV.MuscleMemory
{
	#region PluginInfo
	[PluginInfo(Name = "Ping",
				Category = "MuscleMemory",
				Version = "ID",
				Help = "Ping a motor",
				Tags = "CAN",
				AutoEvaluate = true
		)]
	#endregion PluginInfo
	public class PingMotorIDNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Bus")]
		public ISpread<Bus> FInBus;

		[Input("ID", DefaultValue = 1)]
		public ISpread<ISpread<int>> FInID;

		[Input("Blind")]
		public ISpread<ISpread<bool>> FInBlind;

		[Input("Do", IsBang = true)]
		public ISpread<ISpread<bool>> FInDo;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			for(int iBus=0; iBus < SpreadMax; iBus++)
			{
				var bus = FInBus[iBus];
				if(bus == null)
				{
					continue;
				}

				for(int iID =0; iID< FInID[iBus].SliceCount; iID++)
				{
					if (FInDo[iBus][iID])
					{
						var ID = FInID[iBus][iID];

						if (FInBlind[iBus][iID])
						{
							bus.Send(new Messages.Ping(ID), true);
						}
						else
						{
							var motors = bus.Motors;
							if (motors.ContainsKey(ID))
							{
								motors[ID].Ping();
							}
						}
					}
				}
			}
		}
	}
}
