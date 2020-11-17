using System;
using System.Collections.Generic;
using System.Text;

namespace MuscleMemory
{
	public class Motor
	{
		int ID;
		Bus FBus;
		DateTime FLastSeen = DateTime.MinValue;
		Dictionary<Messages.RegisterType, int> FCachedRegisterValues = new Dictionary<Messages.RegisterType, int>();

		public Motor(int ID, Bus bus)
		{
			this.ID = ID;
			this.FBus = bus;
		}

		public void MarkSeen()
		{
			this.FLastSeen = DateTime.Now;
		}

		public void Process(Messages.ReadResponse readResponse)
		{
			this.FCachedRegisterValues[readResponse.RegisterType] = readResponse.Value;
		}
		
		public void RequestRegister(Messages.RegisterType registerType)
		{
			var readRequest = new Messages.ReadRequest();
			readRequest.ID = this.ID;
			readRequest.RegisterType = registerType;
			this.FBus.Send(readRequest);
		}

		DateTime LastSeen
		{
			get
			{
				return this.FLastSeen;
			}
		}
	}
}
