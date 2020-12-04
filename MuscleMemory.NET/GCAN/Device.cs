using System;
using System.Collections.Generic;
using System.Text;
using System.Collections.Concurrent;
using System.Threading;

namespace GCAN
{
	public class Device : Candle.Device
	{
		UInt32 FDeviceType;
		UInt32 FDeviceIndex;
		DateTime FStartTime;
		
		public Device(UInt32 deviceType, UInt32 deviceIndex)
			: base((IntPtr) 0)
		{
			this.FDeviceIndex = deviceIndex;
			this.FDeviceType = deviceType;
		}

		public override void Dispose()
		{
			this.Close();
		}

		public override void Open()
		{
			this.Close();

			if(NativeFunctions.OpenDevice(this.FDeviceType, this.FDeviceIndex, 0) != NativeFunctions.ECANStatus.STATUS_OK)
			{
				throw (new Exception(String.Format("Failed to open device type ({0}) index ({1})", this.FDeviceType, this.FDeviceIndex)));
			}

			this.FActionQueue = new BlockingCollection<Action>();
			this.FExceptionQueue = new BlockingCollection<Exception>();

			// List Channels
			this.FChannels.Clear();
			byte channelCount = 2; // Currently we only support devices with 2 channels
			for (byte i = 0; i < channelCount; i++)
			{
				this.FChannels.Add(i, new Channel(this, i));
			}

			// Start device thread
			this.FIsClosing = false;
			this.FThread = new Thread(this.ThreadedUpdate);
			this.FThread.Name = String.Format("GCAN {0}:{1}", this.FDeviceType, this.FDeviceIndex);
			this.FThread.Start();

			this.FStartTime = DateTime.Now;
		}

		public override void Close()
		{
			if (this.FThread != null)
			{
				this.PerformBlocking(() =>
				{
					this.FIsClosing = true;
					NativeFunctions.CloseDevice(this.FDeviceType, this.FDeviceIndex);
				});

				// Close queues
				this.FActionQueue.CompleteAdding();
				this.FExceptionQueue.CompleteAdding();

				this.FThread.Join();
				this.FThread = null;

				this.FActionQueue.Dispose();
				this.FActionQueue = null;
				this.FExceptionQueue.Dispose();
				this.FExceptionQueue = null;
			}
		}

		public override string Path
		{
			get
			{
				return String.Format("{0}:{1}", this.FDeviceType, this.FDeviceIndex);
			}
		}

		public override UInt32 Timestamp
		{
			get
			{
				return (UInt32) (DateTime.Now - this.FStartTime).TotalMilliseconds;
			}
		}

		public override Candle.NativeFunctions.candle_devstate_t DeviceState
		{
			get
			{
				if (this.FThread == null)
				{
					return Candle.NativeFunctions.candle_devstate_t.CANDLE_DEVSTATE_AVAIL;
				}
				else
				{
					return Candle.NativeFunctions.candle_devstate_t.CANDLE_DEVSTATE_INUSE;
				}
			}
		}

		void ThreadedUpdate()
		{
			while (!this.FIsClosing)
			{
				try
				{
					// Rx frames
					{
						foreach(var channel in this.FChannels)
						{
							var typedChannel = channel.Value as Channel;
							typedChannel.ReceiveIncoming();
						}
					}

					// Perform actions
					{
						int count = 0;

						Action action;
						while (this.FActionQueue.TryTake(out action))
						{
							action();

							if (count++ > 64)
							{
								// We have trouble when sending more than 92 message in a row
								break;
							}
						}
					}
				}
				catch (Exception e)
				{
					this.FExceptionQueue.Add(e);
				}
			}
		}

		public UInt32 DeviceType
		{
			get
			{
				return this.FDeviceType;
			}
		}

		public UInt32 DeviceIndex
		{
			get
			{
				return this.FDeviceIndex;
			}
		}
	}
}
