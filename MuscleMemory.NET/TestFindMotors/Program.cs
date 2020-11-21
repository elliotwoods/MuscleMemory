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
			var busGroup = new BusGroup();

			Console.WriteLine("Opening bus");
			busGroup.Open(500000);

			Console.WriteLine("Found {0} buses :", busGroup.Buses.Count);
			foreach(var bus in busGroup.Buses)
			{
				Console.WriteLine(bus.DevicePath);
			}


			Console.WriteLine("Finding motors");
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
			if(foundMotors.Count > 0)
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
			
			Console.WriteLine();

			busGroup.Close();
		}
	}
}
