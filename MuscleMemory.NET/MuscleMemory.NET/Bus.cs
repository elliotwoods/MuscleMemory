using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using Candle;

namespace MuscleMemory
{
	class Bus
	{
		Device FDevice;
		Channel FChannel;

		public Bus(Device device)
		{
			this.FDevice = device;
			this.Open();
		}

		~Bus()
		{
			this.Close();
		}

		public void Update()
		{

		}

		public void Open()
		{
			this.FDevice.Open();

			var channels = this.FDevice.Channels;
			if (channels.Count != 1)
			{
				throw (new Exception("This library currently only supports devices with exactly 1 channel"));
			}
			this.FChannel = channels.Values.ToList()[0];
			this.FChannel.Start(500000);

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
			foreach(var reply in replies)
			{
				var message = Messages.Decode(reply);
				if(message is Messages.ReadResponse)
				{
					Console.WriteLine(message);
				}
			}
			Console.WriteLine(replies);
		}
	}
}
