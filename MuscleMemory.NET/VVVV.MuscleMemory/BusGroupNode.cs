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
	[PluginInfo(Name = "BusGroup",
				Category = "MuscleMemory",
				Help = "Get all attached buses",
				Tags = "CAN")]
	#endregion PluginInfo
	public class BusGroupNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Refresh", IsSingle = true, IsBang = true)]
		public ISpread<bool> FInRefresh;

		[Input("Bitrate", IsSingle = true, DefaultValue = 500000)]
		public IDiffSpread<int> FInBitrate;

		[Output("BusGroup")]
		public ISpread<BusGroup> FOutBusgroup;

		[Output("Motor ID")]
		public ISpread<int> FOutMotorID;

		[Output("Motors")]
		public ISpread<Motor> FOutMotors;

		[Output("Buses")]
		public ISpread<Bus> FOutBuses;

		[Output("IsOpen")]
		public ISpread<bool> FOutIsOpen;

		[Import()]
		public ILogger FLogger;

		BusGroup FBusGroup = null;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			if ((FInRefresh[0] || FInBitrate.IsChanged) && this.FBusGroup != null)
			{
				this.FBusGroup.Close();
			}

			if (FBusGroup == null)
			{
				this.FBusGroup = new BusGroup(this.FInBitrate[0]);
			}

			if (!FBusGroup.IsOpen)
			{
				this.FBusGroup.Open(this.FInBitrate[0]);
			}

			this.FOutBusgroup[0] = this.FBusGroup;

			this.FOutBuses.AssignFrom(this.FBusGroup.Buses);

			var motors = this.FBusGroup.Motors;
			this.FOutMotorID.AssignFrom(motors.Keys);
			this.FOutMotors.AssignFrom(motors.Values);

			if(this.FBusGroup == null)
			{
				this.FOutIsOpen[0] = false;
			}
			else
			{
				this.FOutIsOpen[0] = this.FBusGroup.IsOpen;
			}
		}
	}
}
