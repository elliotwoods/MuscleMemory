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
	[PluginInfo(Name = "BusGroup",
				Category = "MuscleMemory",
				Help = "Get all attached buses",
				Tags = "CAN",
				AutoEvaluate = true
		)]
	#endregion PluginInfo
	public class BusGroupNode : IPluginEvaluate, IDisposable
	{
		static bool FAlternativeDevicesRegistered = false;

		#region fields & pins
		[Input("Refresh", IsSingle = true, IsBang = true)]
		public ISpread<bool> FInRefresh;

		[Input("Restart", IsSingle = true, IsBang = true)]
		public ISpread<bool> FInRestart;

		[Input("Bitrate", IsSingle = true, DefaultValue = 500000)]
		public IDiffSpread<int> FInBitrate;

		[Input("Enabled", IsSingle = true)]
		public ISpread<bool> FInEnabled;

		[Output("BusGroup")]
		public ISpread<BusGroup> FOutBusgroup;

		[Output("Buses")]
		public ISpread<Bus> FOutBuses;

		[Output("Motor ID")]
		public ISpread<int> FOutMotorID;

		[Output("Motors")]
		public ISpread<Motor> FOutMotors;

		[Output("IsOpen")]
		public ISpread<bool> FOutIsOpen;

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
			if (!FAlternativeDevicesRegistered)
			{
				GCAN.Initializer.RegisterDevices();
				FAlternativeDevicesRegistered = true;
			}

			if (this.FBusGroup == null)
			{
				this.FBusGroup = new BusGroup();
			}

			if (FInRestart[0] || FInBitrate.IsChanged)
			{
				this.FBusGroup.Close();
			}

			if (!FBusGroup.IsOpen && FInEnabled[0])
			{
				this.FBusGroup.Open(this.FInBitrate[0]);
				this.FBusGroup.Refresh();
			}
			else if(FBusGroup.IsOpen && !FInEnabled[0])
			{
				this.FBusGroup.Close();
			}

			if(FInRefresh[0] && this.FBusGroup.IsOpen)
			{
				this.FBusGroup.Refresh();
			}

			this.FOutBusgroup[0] = this.FBusGroup;
			this.FOutIsOpen[0] = this.FBusGroup.IsOpen;

			if (FBusGroup.IsOpen)
			{
				// This must be called regularly (e.g. once per mainloop frame)
				this.FBusGroup.Update();

				this.FOutBuses.AssignFrom(this.FBusGroup.Buses);
				var motors = this.FBusGroup.GetAllMotors();
				this.FOutMotorID.AssignFrom(motors.Keys);
				this.FOutMotors.AssignFrom(motors.Values);
			}
			else
			{
				this.FOutBuses.SliceCount = 0;
				this.FOutMotors.SliceCount = 0;
				this.FOutMotorID.SliceCount = 0;
			}
		}
	}
}
