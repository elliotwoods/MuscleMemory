using System;
using System.Threading;
using System.Threading.Tasks;
using MuscleMemory;

namespace TestApp
{
	class Program
	{
		const int maxMotorID = 80;
		static UInt64 txCountTotal = 0;

		static void ForceSendToAll(BusGroup busGroup, Messages.RegisterType registerType, int value, bool blocking)
		{
			var writeRequest = new Messages.WriteRequest(1, registerType, value);

			for (int i = 1; i <= maxMotorID; i++)
			{
				writeRequest.ID = i;
				Parallel.ForEach(busGroup.Buses, (bus) =>
				{
					bus.Send(writeRequest, blocking);
					Program.txCountTotal++;
				});
			}
		}

		static void ForceSendToAllPrimary(BusGroup busGroup, int value, bool blocking)
		{
			var writeRequest = new Messages.WritePrimaryRegisterRequest(1, value);

			for (int i = 1; i <= maxMotorID; i++)
			{
				writeRequest.ID = i;
				Parallel.ForEach(busGroup.Buses, (bus) =>
				{
					bus.Send(writeRequest, blocking);
					Program.txCountTotal++;
				});
			}
		}



		static void Main(string[] args)
		{
			var busGroup = new BusGroup();

			Console.WriteLine("Opening bus...");
			busGroup.Open(500000);

			// Refresh the motors (for debugging)
			busGroup.Refresh();

			{
				var motors = busGroup.GetAllMotors();
				Console.WriteLine("Found {0} motors", motors.Count);
				if(motors.Count == 0)
				{
					busGroup.Close();
					return;
				}
			}

			// Enable Torque
			ForceSendToAll(busGroup, Messages.RegisterType.ControlMode, 1, true);

			// Disable screen (movements are smoother)
			ForceSendToAll(busGroup, Messages.RegisterType.InterfaceEnabled, 0, true);

			// Perform movement N times
			int N = 1000;
			for(int n = 0; n<N; n++)
			{
				Console.WriteLine("Iteration {0}/{1}...", n, N);
				int i = 0;
				int limit = 1 << 10;
				int amp = 13;
				for (; i < limit; i++)
				{
					var pos = i << amp;
					if(i % 4 == 0)
					{
						Console.WriteLine("Moving to {0}/{1}", pos, limit << amp);
					}
					ForceSendToAllPrimary(busGroup, pos, false);
					Thread.Sleep(10);
				}
				for (; i >= 0; i--)
				{
					var pos = i << amp;
					if (i % 4 == 0)
					{
						Console.WriteLine("Moving to {0}/{1}", pos, limit << amp);
					}
					ForceSendToAllPrimary(busGroup, pos, false);
					Thread.Sleep(10);
				}

				Console.WriteLine("Wrote {0} messages total", txCountTotal);
			}

			Console.WriteLine("Closing bus...");

			busGroup.Close();
		}
	}
}
