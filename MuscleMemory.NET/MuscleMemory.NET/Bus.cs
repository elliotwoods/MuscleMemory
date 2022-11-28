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
		Channel FChannel;
		int FBitrate;
		string FDevicePath;
		UInt32 FTimestamp = 0;
		int FRxCountThisFrame = 0;
		SortedDictionary<int, Motor> FMotors = new SortedDictionary<int, Motor>();

		object FLockIO = new object();

		public Bus(Channel channel)
		{
			this.FChannel = channel;
		}

		~Bus()
		{
			this.Stop();
		}

		public void Update()
		{
			if (this.IsOpen)
			{
				this.FTimestamp = this.FChannel.Device.Timestamp;

				// Decode incoming messages
				var frames = this.FChannel.Receive();
				this.FRxCountThisFrame = frames.Count;
				foreach(var frame in frames)
				{
					var message = Messages.Decode(frame);
					if(message is Messages.PingResponse)
					{
						var pingResponse = message as Messages.PingResponse;

						var ID = pingResponse.ID;

						this.NotifyMotorExists(ID);
					}
					else if(message is Messages.ReadResponse)
					{
						var readResponse = message as Messages.ReadResponse;

						// We found a motor
						var ID = readResponse.ID;

						var motor = this.NotifyMotorExists(ID);
						motor.Receive(readResponse);
					}
				}
			}
		}

		public Motor NotifyMotorExists(int ID)
		{
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
			motor.MarkSeen();
			return motor;
		}

		public void Start(int bitrate)
		{
			this.FBitrate = bitrate;
			this.FDevicePath = this.FChannel.Device.Path;
			this.FChannel.Start(bitrate);
		}

		public void Stop()
		{
			if(this.IsOpen)
			{
				this.FChannel.Stop();
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

		public void SendRefresh(int maxIndex = Messages.MaxIndex)
		{
			lock (this.FLockIO)
			{
				this.FMotors.Clear();
				// Send the request to all indexes
				try
				{
					for (int i = 1; i <= maxIndex; i++)
					{
						var message = new Messages.Ping(i);
						this.FChannel.Send(message.Encode());
					}

					this.Channel.Device.BlockUntilActionsComplete(new TimeSpan(0, 0, 5));
				}
				catch (Exception e)
				{
					Console.WriteLine(e);
				}
			}
		}

		public void Send(Messages.IMessage message, bool blocking)
		{
			lock(this.FLockIO)
			{
				var frame = message.Encode();
				this.FChannel.Send(frame, blocking);
			}
		}

		public Channel Channel
		{
			get
			{
				return this.FChannel;
			}
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

		public double BusTraffic
		{
			get
			{
				return (double)(this.FChannel.RxBitsPerSecond + this.FChannel.TxBitsPerSecond) / (double)this.FBitrate;
			}
		}

		public int RxCountThisFrame
		{
			get
			{
				return this.FRxCountThisFrame;
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

		public int TxQueueSize
		{
			get
			{
				return this.FChannel.Device.ActionQueueSize;
			}
		}

		public override string ToString()
		{
			return String.Format("Path : {0}, Motors : {1}", this.FDevicePath, this.FMotors.Count);
		}
	}
}
