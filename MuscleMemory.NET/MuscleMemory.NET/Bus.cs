using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using Candle;

namespace MuscleMemory
{
	public class Bus
	{
		Device FDevice;
		int FBitrate;
		Channel FChannel;
		UInt32 FTimestamp = 0;
		Dictionary<int, Motor> FMotors = new Dictionary<int, Motor>();

		public Bus(Device device, int bitrate)
		{
			this.FDevice = device;
			this.Open(bitrate);
		}

		~Bus()
		{
			this.Close();
		}

		public void Update()
		{
			if (this.IsOpen)
			{
				this.FDevice.Perform(() =>
				{
					this.FTimestamp = this.FDevice.Timestamp;
				});
				this.FDevice.Update();
			}
		}

		public void Open(int bitrate)
		{
			this.FBitrate = bitrate;
			this.FDevice.Open();

			var channels = this.FDevice.Channels;
			if (channels.Count != 1)
			{
				throw (new Exception("This library currently only supports devices with exactly 1 channel"));
			}
			this.FChannel = channels.Values.ToList()[0];
			this.FChannel.Start(bitrate);

			this.Refresh();
		}

		public void Close()
		{
			if(this.IsOpen)
			{
				this.FChannel.Stop();
				this.FDevice.Close();
				this.FDevice = null;
			}
		}

		public bool IsOpen
		{
			get
			{
				return this.FDevice != null;
			}
		}

		public void Refresh(int timeout = 1000)
		{
			// Send the request to all indexes
			for (int i = 1; i < Messages.MaxIndex; i++)
			{
				var message = new Messages.ReadRequest();
				message.ID = i;
				message.RegisterType = Messages.RegisterType.DeviceID;
				this.FChannel.Send(message.Encode());
			}

			// Wait for responses
			Thread.Sleep(timeout);
			var replies = this.FChannel.Receive();

			// Rebuild our dictionary of motors
			var priorData = this.FMotors;
			var newData = new Dictionary<int, Motor>();
			foreach(var reply in replies)
			{
				var message = Messages.Decode(reply);
				if(message is Messages.ReadResponse)
				{
					var readResponse = message as Messages.ReadResponse;

					// We found a motor
					var ID = readResponse.ID;
					Motor motor;
					if(priorData.ContainsKey(ID))
					{
						motor = priorData[ID];
					}
					else
					{
						motor = new Motor(readResponse.ID, this);
					}
					motor.MarkSeen();
					motor.Process(readResponse);
					newData.Add(ID, motor);
				}
			}
			this.FMotors = newData;
		}

		public void Send(Messages.IMessage message)
		{
			var frame = message.Encode();
			this.FChannel.Send(frame);
		}

		public Dictionary<int, Motor> Motors
		{
			get
			{
				return this.FMotors;
			}
		}

		public UInt32 Timestamp
		{
			get
			{
				return this.FTimestamp;
			}
		}

		public Device Device
		{
			get
			{
				return this.FDevice;
			}
		}

		public double BusTraffic
		{
			get
			{
				return (double)(this.FDevice.RxBitsPerSecond + this.FDevice.TxBitsPerSecond) / (double)this.FBitrate;
			}
		}
	}
}
