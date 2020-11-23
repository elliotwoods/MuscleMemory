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
	[PluginInfo(Name = "MotorListTest",
				Category = "MuscleMemory",
				Help = "Get info for Motor",
				Tags = "CAN")]
	#endregion PluginInfo
	public class MotorListTestNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Refresh")]
		public ISpread<bool> FInRefresh;

		[Output("Bus Group")]
		public ISpread<BusGroup> FOutBusGroup;

		[Output("Motor ID")]
		public ISpread<ISpread<int>> FOutMotorID;

		[Import()]
		public ILogger FLogger;

		BusGroup FBusGroup = null;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			if(this.FBusGroup == null)
			{
				this.FBusGroup = new BusGroup();
				this.FBusGroup.Open(500000);
				this.FBusGroup.Refresh();
			}
			else
			{
				if(this.FInRefresh[0])
				{
					this.FBusGroup.Close();
					this.FBusGroup = null;
				}
				else
				{
					this.FOutMotorID.SliceCount = this.FBusGroup.Buses.Count;
					for(int i=0; i<this.FBusGroup.Buses.Count; i++)
					{
						this.FOutMotorID[i].AssignFrom(this.FBusGroup.Buses[i].Motors.Keys);
					}
				}
			}

			this.FOutBusGroup[0] = this.FBusGroup;
		}
	}
}
