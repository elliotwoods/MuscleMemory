using System;
using System.Collections.Generic;
using Candle;

namespace MuscleMemory
{
	public class BusGroup
	{
		int FBitrate;
		List<Bus> FBuses;

		public BusGroup(int bitrate = 500000)
		{
			this.Open(bitrate);
		}

		~BusGroup()
		{
			this.Close();
		}

		public void Open(int bitrate)
		{
			this.FBitrate = bitrate;
			this.Close();
			var devices = Device.ListDevices();
			var buses = new List<Bus>();
			foreach(var device in devices)
			{
				try
				{
					buses.Add(new Bus(device, bitrate));
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
			this.Open(this.FBitrate);
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

		public Dictionary<int, Motor> Motors
		{
			get
			{
				var motors = new Dictionary<int, Motor>();
				foreach(var bus in this.FBuses)
				{
					foreach(var iterator in bus.Motors)
					{
						motors[iterator.Key] = iterator.Value;
					}
				}
				return motors;
			}
		}
	}
}
