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
	[PluginInfo(Name = "GetRegisterValue",
				Category = "MuscleMemory",
				Help = "Poll data for register values",
				Version ="Group",
				Tags = "CAN")]
	#endregion PluginInfo
	public class GetRegisterValueNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Bus Group", IsSingle = true)]
		public ISpread<BusGroup> FInBusGorup;

		[Input("ID", DefaultValue = 1)]
		public ISpread<int> FInID;

		[Input("Register Type")]
		public ISpread<Messages.RegisterType> FInRegisterType;

		[Input("Send Requests", IsSingle = true)]
		public ISpread<bool> FInSendRequests;

		[Output("Motor Present")]
		public ISpread<bool> FOutMotorPresent;

		[Output("Last Seen")]
		public ISpread<string> FOutLastSeen;

		[Output("Values")]
		public ISpread<ISpread<int>> FOutValues;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			FOutMotorPresent.SliceCount = FInID.SliceCount;
			FOutLastSeen.SliceCount = FInID.SliceCount;
			FOutValues.SliceCount = FInID.SliceCount;

			if(FInBusGorup.SliceCount != 1 || FInBusGorup[0] == null)
			{
				// No bus attached
				FOutMotorPresent.SliceCount = 0;
				FOutLastSeen.SliceCount = 0;
				FOutValues.SliceCount = 0;
			}
			else
			{
				var motors = FInBusGorup[0].GetAllMotors();
				for (int i = 0; i < FInID.SliceCount; i++)
				{
					var motorPresent = motors.ContainsKey(FInID[i]);
					FOutMotorPresent[i] = motorPresent;
					if (motorPresent)
					{
						var motor = motors[FInID[i]];

						// Perform poll
						if(FInSendRequests[0])
						{
							foreach(var registerType in FInRegisterType)
							{
								motor.RequestRegister(registerType, false);
							}
						}

						// Fill output
						FOutLastSeen[i] = motor.LastSeen.ToString();
						FOutValues[i].SliceCount = FInRegisterType.SliceCount;

						var registerValues = motor.CachedRegisterValues;
						for(int j=0; j<FInRegisterType.SliceCount; j++)
						{
							if (registerValues.ContainsKey(FInRegisterType[j]))
							{
								FOutValues[i][j] = registerValues[FInRegisterType[j]];
							}
							else
							{
								FOutValues[i][j] = 0;
							}
						}
					}
					else
					{
						FOutLastSeen[i] = "";
						FOutValues[i].SliceCount = 0;
					}
				}
			}
			
		}
	}
}
