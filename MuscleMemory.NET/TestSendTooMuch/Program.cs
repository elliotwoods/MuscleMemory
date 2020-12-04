using System;
using System.Threading;
using System.Threading.Tasks;
using MuscleMemory;

namespace TestApp
{
	class Program
	{
		static void Main(string[] args)
		{
			UInt64 txCountTotal = 0;
			UInt64 rxCountTotal = 0;

			// Also get GCAN devices
			GCAN.Initializer.RegisterDevices();

			var busGroup = new BusGroup();

			Console.WriteLine("Opening bus...");
			busGroup.Open(500000);

			if(busGroup.Buses.Count < 1)
			{
				throw (new Exception("No buses found"));
			}

			var bus = busGroup.Buses[0];
			Console.WriteLine("Timestamp : {0}", bus.Channel.Device.Timestamp);

			// Disable Torque and enable screen (watch the screen to notice messaging)
			Console.WriteLine("Setting up for run");
			busGroup.SetRegisterValueBlind(1, Messages.RegisterType.ControlMode, 0, true);
			busGroup.SetRegisterValueBlind(1, Messages.RegisterType.InterfaceEnabled, 1, true);


			var writeRequest = new Messages.WriteRequest(1, Messages.RegisterType.TargetPosition, 0);

			Console.WriteLine("Sending...");
			while (true)
			{
				for(int i=0; i<100; i++)
				{
					// Send to device 1
					writeRequest.ID = 1;
					writeRequest.Value = i << 14;
					bus.Send(writeRequest, false);
					txCountTotal += 1;

					// Send to absent devices
					for(int j=0; j<256; j++)
					{
						writeRequest.ID = 2;
						bus.Send(writeRequest, false);
						txCountTotal += 1;
					}
				}

				busGroup.BlockUntilActionsComplete();
				busGroup.Update();
				rxCountTotal += (UInt64)bus.RxCountThisFrame;
				
				Console.WriteLine("Sent {0} frames. Received {1} frames", txCountTotal, rxCountTotal);
			}
			

			Console.WriteLine("Closing bus...");

			busGroup.Close();
		}
	}
}
