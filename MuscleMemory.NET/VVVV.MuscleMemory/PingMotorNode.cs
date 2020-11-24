using MuscleMemory;
using System.ComponentModel.Composition;
using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V2;

namespace VVVV.MuscleMemory
{
	#region PluginInfo
	[PluginInfo(Name = "Ping",
				Category = "MuscleMemory",
				Version = "Motor",
				Help = "Ping a motor",
				Tags = "CAN",
				AutoEvaluate = true
		)]
	#endregion PluginInfo
	public class PingMotorNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Motor")]
		public ISpread<Motor> FInMotor;

		[Input("Do", IsBang = true)]
		public ISpread<bool> FInDo;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			for (int i = 0; i < SpreadMax; i++)
			{
				if (FInDo[i])
				{
					if (FInMotor[i] != null)
					{
						FInMotor[i].Ping();
					}
				}
			}
		}
	}
}
