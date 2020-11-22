using System;
using System.Collections.Generic;
using System.Text;

namespace MuscleMemory
{
	public class Motor
	{
		int FID;
		Bus FBus;
		DateTime FLastSeen = DateTime.MinValue;
		Dictionary<Messages.RegisterType, int> FCachedRegisterValues = new Dictionary<Messages.RegisterType, int>();

		public Motor(int ID, Bus bus)
		{
			this.FID = ID;
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
		
		public void RequestRegister(Messages.RegisterType registerType, bool blocking)
		{
			var readRequest = new Messages.ReadRequest();
			readRequest.ID = this.ID;
			readRequest.RegisterType = registerType;
			this.FBus.Send(readRequest, blocking);
		}

		public void Receive(Messages.ReadResponse readResponse)
		{
			this.MarkSeen();
			this.FCachedRegisterValues[readResponse.RegisterType] = readResponse.Value;
		}

		public int ID
		{
			get
			{
				return this.FID;
			}
		}
		public DateTime LastSeen
		{
			get
			{
				return this.FLastSeen;
			}
		}

		public Bus Bus
		{
			get
			{
				return this.FBus;
			}
		}

		public Dictionary<Messages.RegisterType, int> CachedRegisterValues
		{
			get
			{
				return this.FCachedRegisterValues;
			}
		}

		public void SetRegister(Messages.RegisterType registerType, int value, bool blocking)
		{
			var writeRequest = new Messages.WriteRequest();
			writeRequest.ID = this.ID;
			writeRequest.RegisterType = registerType;
			writeRequest.Value = value;
			this.FBus.Send(writeRequest, blocking);
		}

		public void SetPrimaryRegister(int value, bool blocking)
		{
			var writeRequest = new Messages.WritePrimaryRegisterRequest();
			writeRequest.ID = this.ID;
			writeRequest.Value = value;
			this.FBus.Send(writeRequest, blocking);
		}
	}
}
