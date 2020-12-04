using System;
using System.Collections.Generic;
using System.Text;

namespace GCAN
{
	public class Channel : Candle.Channel
	{
		Device FDevice;
		UInt32 FChannelIndex;

		public Channel(Device device, UInt32 channelIndex)
			: base(device, (byte) channelIndex)
		{
			this.FDevice = device;
			this.FChannelIndex = channelIndex;
		}

		public override Candle.NativeFunctions.candle_capability_t Capabilities
		{
			get
			{
				var value = new Candle.NativeFunctions.candle_capability_t();
				return value;
			}
		}

		public override void SetTiming(Candle.NativeFunctions.candle_bittiming_t value)
		{
			throw (new Exception("Cannot call set timing"));
		}

		public override void SetBitrate(int value)
		{
			var initConfig = new NativeFunctions.INIT_CONFIG();
			initConfig.AccCode = 0;
			initConfig.AccMask = 0xffffff;
			initConfig.Filter = 0;

			switch (value)
			{
				case 1000000:
					initConfig.Timing0 = 0;
					initConfig.Timing1 = 0x14;
					break;
				case 800000:
					initConfig.Timing0 = 0;
					initConfig.Timing1 = 0x16;
					break;
				case 666000:
					initConfig.Timing0 = 0x80;
					initConfig.Timing1 = 0xb6;
					break;
				case 500000:
					initConfig.Timing0 = 0;
					initConfig.Timing1 = 0x1c;
					break;
				case 400000:
					initConfig.Timing0 = 0x80;
					initConfig.Timing1 = 0xfa;
					break;
				case 250000:
					initConfig.Timing0 = 0x01;
					initConfig.Timing1 = 0x1c;
					break;
				case 200000:
					initConfig.Timing0 = 0x81;
					initConfig.Timing1 = 0xfa;
					break;
				case 125000:
					initConfig.Timing0 = 0x03;
					initConfig.Timing1 = 0x1c;
					break;
				case 100000:
					initConfig.Timing0 = 0x04;
					initConfig.Timing1 = 0x1c;
					break;
				case 80000:
					initConfig.Timing0 = 0x83;
					initConfig.Timing1 = 0xff;
					break;
				case 50000:
					initConfig.Timing0 = 0x09;
					initConfig.Timing1 = 0x1c;
					break;
				default:
					throw (new Exception("Bitrate not supported"));
			}

			initConfig.Mode = 0;

			this.FDevice.PerformBlocking(() =>
			{
				NativeFunctions.ThrowIfError(
					NativeFunctions.InitCAN(
						this.FDevice.DeviceType
						, this.FDevice.DeviceIndex
						, this.FChannelIndex
						, ref initConfig)

					, this.FDevice.DeviceType
					, this.FDevice.DeviceIndex
					, this.FChannelIndex); ;
			});
		}

		public override void Start(int bitrate)
		{
			this.SetBitrate(bitrate);
			this.FDevice.PerformBlocking(() =>
			{
				NativeFunctions.ThrowIfError(
					NativeFunctions.StartCAN(
						this.FDevice.DeviceType
						, this.FDevice.DeviceIndex
						, this.FChannelIndex)

					, this.FDevice.DeviceType
					, this.FDevice.DeviceIndex
					, this.FChannelIndex);
			});

			this.FCounterLastTime = DateTime.Now;
			this.FCounterRxBits = 0;
			this.FCounterTxBits = 0;
		}

		public override void Stop()
		{
			this.FDevice.PerformBlocking(() =>
			{
				NativeFunctions.ThrowIfError(
					NativeFunctions.ResetCAN(
						this.FDevice.DeviceType
						, this.FDevice.DeviceIndex
						, this.FChannelIndex)

					, this.FDevice.DeviceType
					, this.FDevice.DeviceIndex
					, this.FChannelIndex);
			});
		}

		public override void Send(Candle.Frame frame, bool blocking = false)
		{
			var nativeFrame = new NativeFunctions.CAN_OBJ();
			nativeFrame.ID = frame.Identifier;
			
			if (frame.Extended)
			{
				nativeFrame.ExternFlag = 1;
			}
			else
			{
				nativeFrame.ExternFlag = 0;
			}

			if (frame.RTR)
			{
				nativeFrame.RemoteFlag = 1;
			}
			else
			{
				nativeFrame.RemoteFlag = 0;
			}

			nativeFrame.data = new byte[8];
			nativeFrame.DataLen = (byte) frame.Data.Length;
			Buffer.BlockCopy(frame.Data, 0, nativeFrame.data, 0, frame.Data.Length);

			var lengthOnBus = frame.LengthOnBus;

			this.FDevice.PerformInRightThread(() =>
			{
				try
				{
					NativeFunctions.ThrowIfError(
						NativeFunctions.Transmit(
							this.FDevice.DeviceType
							, this.FDevice.DeviceIndex
							, this.FChannelIndex
							, new NativeFunctions.CAN_OBJ[] { nativeFrame }
							, 1)

						, this.FDevice.DeviceType
						, this.FDevice.DeviceIndex
						, this.FChannelIndex);
					this.IncrementTx(lengthOnBus);

					this.ReceiveIncoming();
				}
				catch(Exception e)
				{
					this.Device.NotifyError(e);
				}

			}, blocking);
		}

		public void ReceiveIncoming()
		{
			NativeFunctions.CAN_OBJ nativeFrame;
			while (NativeFunctions.Receive(this.FDevice.DeviceType
				, this.FDevice.DeviceIndex
				, this.FChannelIndex
				, out nativeFrame
				, 1
				, 0) == NativeFunctions.ECANStatus.STATUS_OK)
			{
				var frame = new Candle.Frame();
				{
					frame.Identifier = nativeFrame.ID;
					frame.Extended = nativeFrame.ExternFlag == 1;
					frame.RTR = nativeFrame.RemoteFlag == 1;
					frame.Error = false; // Not sure how we get error frames

					frame.Data = new byte[nativeFrame.DataLen];
					Buffer.BlockCopy(nativeFrame.data, 0, frame.Data, 0, nativeFrame.DataLen);

					frame.Timestamp = nativeFrame.TimeStamp;
				}

				this.IncrementRx(frame.LengthOnBus);
				this.FRxQueue.Add(frame);
			}
		}
	}
}
