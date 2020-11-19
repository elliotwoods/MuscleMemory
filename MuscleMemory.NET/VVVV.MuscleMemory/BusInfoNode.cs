using MuscleMemory;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Text;
using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V2;

namespace VVVV.MuscleMemory
{
#region PluginInfo
	[PluginInfo(Name = "BusInfo",
				Category = "MuscleMemory",
				Help = "Get info for bus",
				Tags = "CAN")]
#endregion PluginInfo
	public class BusInfoNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Bus")]
		public ISpread<Bus> FInput;

		[Input("Refresh", IsBang = true)]
		public ISpread<bool> FInRefresh;

		[Output("Motor IDs")]
		public ISpread<ISpread<int>> FOutMotorIDs;

		[Output("Motors")]
		public ISpread<ISpread<Motor>> FOutMotors;

		[Output("Path")]
		public ISpread<string> FOutPath;

		[Output("Timestamp")]
		public ISpread<int> FOutTimestamp;

		[Output("Bitrate")]
		public ISpread<int> FOutBitrate;

		[Output("Bus Traffic")]
		public ISpread<double> FOutBusTraffic;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			FOutMotorIDs.SliceCount = 0;
			FOutMotors.SliceCount = 0; 
			FOutPath.SliceCount = 0;
			FOutTimestamp.SliceCount = 0;
			FOutBitrate.SliceCount = 0;
			FOutBusTraffic.SliceCount = 0;

			for(int i=0; i<SpreadMax; i++)
			{
				var bus = FInput[i];
				if(bus != null)
				{
					if(FInRefresh[i])
					{
						bus.SendRefresh();
					}

					FOutMotorIDs.Add(bus.Motors.Keys.ToSpread());
					FOutMotors.Add(bus.Motors.Values.ToSpread());
					FOutPath.Add(bus.DevicePath);
					FOutTimestamp.Add((int)bus.Timestamp);
					FOutBitrate.Add(bus.Bitrate);
					FOutBusTraffic.Add(bus.BusTraffic);
				}
			}
		}
	}
}
