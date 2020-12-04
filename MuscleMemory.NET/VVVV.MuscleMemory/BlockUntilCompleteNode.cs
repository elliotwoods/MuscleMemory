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
	[PluginInfo(Name = "BlockUntilComplete",
				Category = "MuscleMemory",
				Help = "Block until all Tx and Action buffers are complete",
				Tags = "CAN",
				AutoEvaluate = true
		)]
	#endregion PluginInfo
	public class BlockUntilCompleteNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Bus Group", IsSingle = true)]
		public ISpread<BusGroup> FInBusGroup;

		[Input("Timeout", IsSingle = true)]
		public ISpread<int> FInTimeout;

		[Input("Enabled", IsSingle = true)]
		public ISpread<bool> FInEnabled;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			var busGroup = FInBusGroup[0];
			if (busGroup is BusGroup && FInEnabled[0])
			{
				busGroup.BlockUntilActionsComplete(new TimeSpan(0, 0, FInTimeout[0]));
			}
		}
	}
}
