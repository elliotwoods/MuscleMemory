using System;
using MuscleMemory;

namespace TestApp
{
	class Program
	{
		static void Main(string[] args)
		{
			var busGroup = new BusGroup();

			Console.WriteLine("Opening bus...");
			busGroup.Open(500000);

			Console.WriteLine("Found motors : ");
			var motors = busGroup.GetAllMotors();
			foreach(var it in motors)
			{
				Console.WriteLine("{0} : {1}", it.Key, it.Value);
			}

			busGroup.Close();
		}
	}
}
