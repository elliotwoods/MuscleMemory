using System;
using System.Threading;
using System.Threading.Tasks;
using MuscleMemory;

namespace TestApp
{
	class Program
	{
		static void ForceSendToAll(BusGroup busGroup, Messages.RegisterType registerType, int value, bool wait)
		{
			var writeRequest = new Messages.WriteRequest();
			writeRequest.RegisterType = registerType;
			writeRequest.Value = value;

			for (int i=0; i<=80; i++)
			{
				writeRequest.ID = i;
				Parallel.ForEach(busGroup.Buses, (bus) =>
				{
					bus.Send(writeRequest, true);
					if (wait)
					{
						Thread.Sleep(1);
					}
				});
			}
		}

		static void Main(string[] args)
		{
			var busGroup = new BusGroup();

			Console.WriteLine("Opening bus...");
			busGroup.Open(500000);

			//Console.WriteLine("Found motors : ");
			//var motors = busGroup.GetAllMotors();
			//foreach(var it in motors)
			//{
			//	Console.WriteLine("{0} : {1}", it.Key, it.Value);
			//}

			ForceSendToAll(busGroup, Messages.RegisterType.ControlMode, 1, true);

			int i = 0;
			int limit = 1 << 10;
			int amp = 9;
			for (; i<limit; i++)
			{
				var pos = i << amp;
				Console.WriteLine("Moving to {0}", pos);
				ForceSendToAll(busGroup, Messages.RegisterType.TargetPosition, pos, false);
			}
			for (; i >= 0; i--)
			{
				var pos = i << amp;
				Console.WriteLine("Moving to {0}", pos);
				ForceSendToAll(busGroup, Messages.RegisterType.TargetPosition, pos, false);
			}

			Console.WriteLine("Closing bus...");

			busGroup.Close();
		}
	}
}
