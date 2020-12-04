using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace GCAN
{
	public class Channel : Candle.Channel
	{
		Device FDevice;
		UInt32 FChannelIndex;
		protected BlockingCollection<NativeFunctions.CAN_OBJ> FTxQueue = new BlockingCollection<NativeFunctions.CAN_OBJ>();

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

			// Note : this shoud actually increment after we've actully sent (e.g. on thread). But since it's a heuristic this is kind of fine for now
			var lengthOnBus = frame.LengthOnBus;
			this.IncrementTx(lengthOnBus);

			if (blocking)
			{
				
				// Send this one frame
				this.FDevice.PerformInRightThread(() =>
				{
					NativeFunctions.Transmit(
						this.FDevice.DeviceType
						, this.FDevice.DeviceIndex
						, this.FChannelIndex
						, new NativeFunctions.CAN_OBJ[] { nativeFrame }
						, 1);

					this.ThreadedUpdate();
				}, true);
			}
			else
			{
				this.FTxQueue.Add(nativeFrame);
			}
		}

		public void ThreadedUpdate()
		{
			// Send frames
			{
				var framesToSend = new List<NativeFunctions.CAN_OBJ>();

				// Pull them from the queue
				{
					NativeFunctions.CAN_OBJ frame;
					while (this.FTxQueue.TryTake(out frame))
					{
						framesToSend.Add(frame);
					}
				}

				// Send them out
				if (framesToSend.Count > 0)
				{
					var framesSent = NativeFunctions.Transmit(
							this.FDevice.DeviceType
							, this.FDevice.DeviceIndex
							, this.FChannelIndex
							, framesToSend.ToArray()
							, (UInt16)framesToSend.Count);

					// Queue again anything that didn't send
					for (int i = (int)framesSent; i < framesToSend.Count; i++)
					{
						this.FTxQueue.Add(framesToSend[i]);
					}
				}

				// Alternative method : force send everything
				//int txOffset = 0;
				//while(txOffset != framesToSend.Count)
				//{
				//	var framesSent = NativeFunctions.Transmit(
				//			this.FDevice.DeviceType
				//			, this.FDevice.DeviceIndex
				//			, this.FChannelIndex
				//			, framesToSend.GetRange(txOffset, framesToSend.Count - txOffset).ToArray()
				//			, (UInt16)framesToSend.Count);
				//	txOffset += (int) framesSent;
				//}
			}

			// Receive frames
			{

				var rxCountOnDevice = NativeFunctions.GetReceiveNum(this.FDevice.DeviceType
					, this.FDevice.DeviceIndex
					, this.FChannelIndex);

				if(rxCountOnDevice > 0)
				{
					var rxBuffer = new NativeFunctions.CAN_OBJ[rxCountOnDevice];

					var rxCount = NativeFunctions.ReceiveArray(this.FDevice.DeviceType
						, this.FDevice.DeviceIndex
						, this.FChannelIndex
						, rxBuffer
						, (UInt32)rxBuffer.Length
						, 0);

					for (int i = 0; i < rxCount; i++)
					{
						var nativeFrame = rxBuffer[i];

						var frame = new Candle.Frame();
						{
							frame.Identifier = nativeFrame.ID;
							frame.Extended = nativeFrame.ExternFlag == 1;
							frame.RTR = nativeFrame.RemoteFlag == 1;
							frame.Error = false; // Not sure how we get error frames with GCAN

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

		public int TxQueueSize
		{
			get
			{
				return this.FTxQueue.Count;
			}
		}
	}
}
