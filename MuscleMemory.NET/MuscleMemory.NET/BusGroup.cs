using System;
using System.Collections.Generic;
using Candle;

namespace MuscleMemory
{
	public class BusGroup
	{
		int FBitrate = 0;
		List<Bus> FBuses = null;

		public BusGroup()
		{
		}

		~BusGroup()
		{
			this.Close();
		}

		public void Open(int bitrate)
		{
			this.Close();
			this.FBuses = new List<Bus>();
			this.FBitrate = bitrate;
			var devices = Device.ListDevices();
			foreach(var device in devices)
			{
				try
				{
					var bus = new Bus(device);
					bus.Open(bitrate);
					bus.Refresh();
					this.FBuses.Add(bus);
				}
				catch(Exception e)
				{
					Console.WriteLine(e);
				}
			}
		}

		public void Close()
		{
			if(this.IsOpen)
			{
				foreach (var bus in this.FBuses)
				{
					bus.Close();
				}
				this.FBuses.Clear();
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

		// Update must be called regularly (e.g. once per mainloop frame)
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

		public List<Bus> Buses
		{
			get
			{
				return this.FBuses;
			}
		}

		public Dictionary<int, Motor> GetAllMotors()
		{
			var motors = new Dictionary<int, Motor>();
			if (this.IsOpen)
			{
				foreach (var bus in this.FBuses)
				{
					foreach (var iterator in bus.Motors)
					{
						motors[iterator.Key] = iterator.Value;
					}
				}
			}
			return motors;
		}

		public Motor FindMotor(int ID)
		{
			foreach (var bus in this.FBuses)
			{
				if(bus.Motors.ContainsKey(ID))
				{
					return bus.Motors[ID];
				}
			}
			return null;
		}
	}
}
