﻿using MuscleMemory;
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
	[PluginInfo(Name = "SetRegisterValue",
				Category = "MuscleMemory",
				Help = "Send data to register values",
				Tags = "CAN",
				AutoEvaluate = true
		)]
	#endregion PluginInfo
	public class SetRegisterValueNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Bus Group", IsSingle = true)]
		public ISpread<BusGroup> FInBusGroup;

		[Input("ID", DefaultValue = 1)]
		public ISpread<int> FInID;

		[Input("Register Type")]
		public ISpread<Messages.RegisterType> FInRegisterType;

		[Input("Value")]
		public ISpread<int> FInValue;

		[Input("Send", IsBang = true)]
		public ISpread<bool> FInSend;

		[Input("Blind")]
		public ISpread<bool> FInBlind;

		[Input("Blocking")]
		public ISpread<bool> FInBlocking;

		[Output("Bus Group")]
		public ISpread<BusGroup> FOutBusGroup;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			FOutBusGroup[0] = FInBusGroup[0];

			var busGroup = FInBusGroup[0];
			if(busGroup is BusGroup)
			{
				// since FInBusGroup is single, we can presume it is ignored for SpreadMax
				for (int i = 0; i < SpreadMax; i++)
				{
					if(FInSend[i])
					{
						if (FInBlind[i])
						{
							FInBusGroup[0].SetRegisterValueBlind(FInID[i], FInRegisterType[i], FInValue[i], FInBlocking[i]);
						}
						else
						{
							var motor = busGroup.FindMotor(FInID[i]);
							if (motor != null)
							{
								motor.SetRegister(FInRegisterType[i], FInValue[i], FInBlocking[i]);
							}
						}
					}
				}
			}
		}
	}
}
