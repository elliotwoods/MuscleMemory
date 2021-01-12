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
		List<Device> FDevices = null;
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
			this.FDevices = new List<Device>();
			this.FBuses = new List<Bus>();
			this.FBitrate = bitrate;
			var devices = Device.ListDevices();
			foreach(var device in devices)
			{
				device.Open();
				var channels = device.Channels;
				foreach (var channelIt in channels)
				{
					var channel = channelIt.Value;
					try
					{
						var bus = new Bus(channel);
						bus.Start(bitrate);
						this.FBuses.Add(bus);
					}
					catch (Exception e)
					{
						Console.WriteLine(e);
					}
				}
				this.FDevices.Add(device);
			}
		}

		public void Close()
		{
			if(this.IsOpen)
			{
				foreach (var bus in this.FBuses)
				{
					bus.Stop();
				}
				this.FBuses.Clear();
				this.FBuses = null;

				foreach(var device in this.FDevices)
				{
					device.Close();
				}
				this.FDevices.Clear();
				this.FDevices = null;
			}
		}

		public bool IsOpen
		{
			get
			{
				return this.FBuses != null;
			}
		}

		public void BlockUntilActionsComplete(TimeSpan timeout)
		{
			Parallel.ForEach(this.FDevices, (device) =>
			{
				device.BlockUntilActionsComplete(timeout);
			});
		}

		public void Restart()
		{
			this.Open(this.FBitrate);
		}

		public List<Exception> GetAllErrors()
		{
			var errors = new List<Exception>();
			foreach(var device in this.FDevices)
			{
				errors.AddRange(device.ReceiveErrors());
			}
			return errors;
		}

		// Update must be called regularly (e.g. once per mainloop frame)
		public void Update()
		{
			foreach (var device in this.FDevices)
			{
				device.Update();
			}

			foreach (var bus in this.FBuses)
			{
				bus.Update();
			}
		}

		public void Refresh(int maxIndex = Messages.MaxIndex)
		{
			Parallel.ForEach(this.FBuses, (bus) =>
			{
				bus.SendRefresh(maxIndex);
			});

			// Allow time for devices to respond
			Thread.Sleep(1000);

			// Flush device queues
			Parallel.ForEach(this.FDevices, (device) =>
			{
				device.BlockUntilActionsComplete(new TimeSpan(0, 0, 0, 10));
			});

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
