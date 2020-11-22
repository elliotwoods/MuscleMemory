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
		string FDevicePath;
		Channel FChannel;
		UInt32 FTimestamp = 0;
		SortedDictionary<int, Motor> FMotors = new SortedDictionary<int, Motor>();

		public Bus(Device device)
		{
			this.FDevice = device;
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
				var frames = this.FChannel.Receive();
				foreach(var frame in frames)
				{
					var message = Messages.Decode(frame);
					if(message is Messages.ReadResponse)
					{
						var readResponse = message as Messages.ReadResponse;

						// We found a motor
						var ID = readResponse.ID;
						Motor motor;
						if (!this.FMotors.ContainsKey(ID))
						{
							motor = new Motor(ID, this);
							this.FMotors[ID] = motor;
						}
						else
						{
							motor = this.FMotors[ID];
						}
						motor.Receive(readResponse);
					}
				}
			}
		}

		public void Open(int bitrate)
		{
			this.FBitrate = bitrate;
			this.FDevice.Open();
			this.FDevicePath = this.FDevice.Path;

			var channels = this.FDevice.Channels;
			if (channels.Count != 1)
			{
				throw (new Exception("This library currently only supports devices with exactly 1 channel"));
			}
			this.FChannel = channels.Values.ToList()[0];
			this.FChannel.Start(bitrate);
		}

		public void Close()
		{
			if(this.IsOpen)
			{
				this.FChannel.Stop();
				this.FDevice.Close();
				this.FChannel = null;
			}
		}

		public bool IsOpen
		{
			get
			{
				return this.FChannel != null;
			}
		}

		public void SendRefresh()
		{
			this.FMotors.Clear();
			// Send the request to all indexes
			for (int i = 1; i < Messages.MaxIndex; i++)
			{
				var message = new Messages.ReadRequest();
				message.ID = i;
				message.RegisterType = Messages.RegisterType.DeviceID;
				this.FChannel.Send(message.Encode(), true);
			}
		}

		public void Send(Messages.IMessage message, bool blocking)
		{
			var frame = message.Encode();
			this.FChannel.Send(frame, blocking);
		}

		public SortedDictionary<int, Motor> Motors
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

		public string DevicePath
		{
			get
			{
				return this.FDevicePath;
			}
		}

		public int Bitrate
		{
			get
			{
				return this.FBitrate;
			}
		}

		public override string ToString()
		{
			return String.Format("Path : {0}, Motors : {1}", this.FDevicePath, this.FMotors.Count);
		}
	}
}
