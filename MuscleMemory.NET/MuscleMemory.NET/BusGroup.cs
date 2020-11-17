using System;
using System.Collections.Generic;
using Candle;

namespace MuscleMemory
{
	public class BusGroup
	{
		List<Bus> FBuses;

		public BusGroup()
		{
			this.Open();
		}

		~BusGroup()
		{
			this.Close();
		}

		public void Open()
		{
			this.Close();
			var devices = Device.ListDevices();
			var buses = new List<Bus>();
			foreach(var device in devices)
			{
				try
				{
					buses.Add(new Bus(device));
				}
				catch(Exception e)
				{
					Console.WriteLine(e);
				}
			}
			this.FBuses = buses;
		}

		public void Close()
		{
			if(this.IsOpen)
			{
				foreach (var bus in this.FBuses)
				{
					bus.Close();
				}
				this.FBuses = null;
			}
		}

		public bool IsOpen
		{
			get
			{
				return this.FBuses != null;
			}
		}

		public void Restart()
		{
			this.Open();
		}

		public void Update()
		{
			foreach(var bus in this.FBuses)
			{
				bus.Update();
			}
		}

		public void Refresh()
		{
			foreach(var bus in this.FBuses)
			{
				bus.Refresh();
			}
		}
	}
}
