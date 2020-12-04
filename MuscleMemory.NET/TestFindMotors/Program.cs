using System;
using System.Linq;
using System.Threading;
using MuscleMemory;

namespace TestFindMotors
{
	class Program
	{
		static void Main(string[] args)
		{
			// Register GCAN
			GCAN.Initializer.RegisterDevices();

			//// do the same as VVVV example
			//{
			//	Console.WriteLine("First try");
			//	var busGroup = new BusGroup();
			//	busGroup.Open(500000);
			//	busGroup.Refresh(80);
			//	var motors = busGroup.GetAllMotors();
			//	Console.WriteLine("Found {0} motors", motors.Count);
			//	busGroup.Close();
			//}

			// do our actual program test
			{
				var busGroup = new BusGroup();

				Console.WriteLine("Opening bus");
				busGroup.Open(500000);

				Console.WriteLine("Found {0} buses :", busGroup.Buses.Count);
				foreach (var bus in busGroup.Buses)
				{
					Console.WriteLine(bus.DevicePath);
				}
				Console.WriteLine();

				Console.WriteLine("Finding motors");
				Console.WriteLine();
				busGroup.Refresh(80);

				// Print found motors
				var foundMotors = busGroup.GetAllMotors();
				Console.WriteLine("Found {0} motors : ", foundMotors.Count);
				foreach (var it in foundMotors)
				{
					Console.WriteLine(it.Value);
				}
				Console.WriteLine();


				// Print gaps in ID range
				var foundIDs = foundMotors.Keys.ToList();
				if (foundMotors.Count > 0)
				{
					var startID = foundIDs[0];
					var endID = foundIDs[foundMotors.Count - 1];
					Console.WriteLine("Missing contiguous IDs between {0} -> {1} (if any):", startID, endID);
					for (int id = startID; id <= endID; id++)
					{
						if (!foundIDs.Contains(id))
						{
							Console.Write("{0}, ", id);
						}
					}
				}

				Console.WriteLine();

				var errors = busGroup.GetAllErrors();
				foreach (var error in errors)
				{
					Console.WriteLine(error);
				}
				busGroup.Close();
			}
		}
	}
}
