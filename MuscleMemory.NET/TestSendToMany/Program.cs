using System;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using MuscleMemory;

namespace TestApp
{
	class Program
	{
		static UInt64 txCountTotal = 0;

		static void SendToAll(BusGroup busGroup, Messages.RegisterType registerType, int value, bool blocking)
		{
			var motors = busGroup.GetAllMotors();
			foreach(var motor in motors.Values)
			{
				motor.SetRegister(registerType, value, blocking);
			}
		}

		static void SendToAllPrimary(BusGroup busGroup, int value, bool blocking)
		{
			var motors = busGroup.GetAllMotors();
			foreach (var motor in motors.Values)
			{
				motor.SetPrimaryRegister(value, blocking);
			}
		}



		static void Main(string[] args)
		{
			var busGroup = new BusGroup();

			Console.WriteLine("Opening bus...");
			busGroup.Open(500000);

			// Refresh the motors (for debugging)
			Console.WriteLine("Finding motors...");
			busGroup.Refresh();

			// Print found motors
			var foundMotors = busGroup.GetAllMotors();
			Console.WriteLine("Found {0} motors : ", foundMotors.Count);
			foreach (var it in foundMotors)
			{
				Console.WriteLine("{0} : {1}", it.Key, it.Value);
			}

			// Print gaps in ID range
			var foundIDs = foundMotors.Keys.ToList();
			if (foundMotors.Count > 0)
			{
				var startID = foundIDs[0];
				var endID = foundIDs[foundMotors.Count - 1];
				Console.WriteLine("Missing contiguous IDs between {0} -> {1}:", startID, endID);
				for (int id = startID; id <= endID; id++)
				{
					if (!foundIDs.Contains(id))
					{
						Console.Write("{0}, ", id);
					}
				}
			}

			Thread.Sleep(2000);

			// Enable Torque
			SendToAll(busGroup, Messages.RegisterType.ControlMode, 1, true);

			// Disable screen (movements are smoother)
			SendToAll(busGroup, Messages.RegisterType.InterfaceEnabled, 0, true);

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
					SendToAllPrimary(busGroup, pos, false);
					Thread.Sleep(10);
				}
				for (; i >= 0; i--)
				{
					var pos = i << amp;
					if (i % 4 == 0)
					{
						Console.WriteLine("Moving to {0}/{1}", pos, limit << amp);
					}
					SendToAllPrimary(busGroup, pos, false);
					Thread.Sleep(10);
				}

				Console.WriteLine("Wrote {0} messages total", txCountTotal);
			}

			Console.WriteLine("Closing bus...");

			busGroup.Close();
		}
	}
}
