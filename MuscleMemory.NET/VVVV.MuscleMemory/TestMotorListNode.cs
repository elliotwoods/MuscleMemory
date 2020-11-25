using MuscleMemory;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Linq;
using System.Text;
using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V2;

namespace VVVV.MuscleMemory
{
	#region PluginInfo
	[PluginInfo(Name = "TestMotorList",
				Category = "MuscleMemory",
				Help = "Get info for Motor",
				Tags = "CAN")]
	#endregion PluginInfo
	public class TestMotorListNode : IPluginEvaluate, IDisposable
	{
		#region fields & pins
		[Input("Reopen", IsBang = true)]
		public ISpread<bool> FInReopen;

		[Input("Refresh", IsBang = true)]
		public ISpread<bool> FInRefresh;

		[Output("Bus Group")]
		public ISpread<BusGroup> FOutBusGroup;

		[Output("ID")]
		public ISpread<int> FOutMotorID;

		[Output("Motor")]
		public ISpread<Motor> FOutMotors;

		[Import()]
		public ILogger FLogger;

		BusGroup FBusGroup = null;

		public void Dispose()
		{
			if(this.FBusGroup != null)
			{
				this.FBusGroup.Close();
				this.FBusGroup = null;
			}
		}
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			if (this.FInReopen[0])
			{
				if(this.FBusGroup != null)
				{
					this.FBusGroup.Close();
					this.FBusGroup = null;
				}
			}

			if (this.FBusGroup == null)
			{
				GC.Collect();
				this.FBusGroup = new BusGroup();
				this.FBusGroup.Open(500000);
				this.FBusGroup.Refresh();
			}

			if(FInRefresh[0])
			{
				this.FBusGroup.Refresh();
			}

			this.FBusGroup.Update();

			var motors = this.FBusGroup.GetAllMotors();

			this.FOutMotorID.AssignFrom(motors.Keys);
			this.FOutMotors.AssignFrom(motors.Values);

			this.FOutBusGroup[0] = this.FBusGroup;
		}
	}
}
