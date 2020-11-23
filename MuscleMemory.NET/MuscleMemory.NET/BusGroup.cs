using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
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
					bus.SendRefresh();
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

		public void BlockUntilActionsComplete()
		{
			Parallel.ForEach(this.FBuses, (bus) =>
			{
				bus.Device.BlockUntilActionsComplete();
			});
		}

		public void Restart()
		{
			this.Open(this.FBitrate);
		}

		public List<Exception> GetAllErrors()
		{
			var errors = new List<Exception>();
			foreach(var bus in this.FBuses)
			{
				errors.AddRange(bus.Device.ReceiveErrors());
			}
			return errors;
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
			Parallel.ForEach(this.FBuses, (bus) =>
			{
				bus.SendRefresh();
			});

			// Allow time for responses (this doesn't seem to be necessary)
			Thread.Sleep(100);
			this.Update();
		}

		public List<Bus> Buses
		{
			get
			{
				return this.FBuses;
			}
		}

		public SortedDictionary<int, Motor> GetAllMotors()
		{
			var motors = new SortedDictionary<int, Motor>();
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

		public void SetRegisterValueBlind(int ID, Messages.RegisterType registerType, int value, bool blocking)
		{
			var message = new Messages.WriteRequest(ID, registerType, value);
			foreach (var bus in this.FBuses)
			{
				bus.Send(message, blocking);
			}
		}

		public void SetPrimaryRegisterValueBlind(int ID, int value, bool blocking)
		{
			var message = new Messages.WritePrimaryRegisterRequest(ID, value);
			foreach(var bus in this.FBuses)
			{
				bus.Send(message, blocking);
			}
		}

		public void PingBlind(int ID)
		{
			var message = new Messages.Ping(ID);
			foreach (var bus in this.FBuses)
			{
				bus.Send(message, false);
			}
		}
	}
}
