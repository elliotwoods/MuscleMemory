using System;
using System.Collections.Generic;
using System.Text;

namespace MuscleMemory
{
	public class Motor
	{
		int FID;
		WeakReference<Bus> FBus;
		DateTime FLastSeen = DateTime.MinValue;
		Dictionary<Messages.RegisterType, int> FCachedRegisterValues = new Dictionary<Messages.RegisterType, int>();

		public delegate void OnRegisterReceiveHandler(Messages.RegisterType register, int value);
		public event OnRegisterReceiveHandler OnRegisterReceive;

		public Motor(int ID, Bus bus)
		{
			this.FID = ID;
			this.FBus = new WeakReference<Bus>(bus, false);
		}

		public Bus Bus
		{
			get
			{
				Bus bus;
				if (!this.FBus.TryGetTarget(out bus))
				{
					throw (new Exception("Bus not valid"));
				}
				else
				{
					return bus;
				}
			}
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
			var readRequest = new Messages.ReadRequest(this.ID, registerType);
			var bus = this.Bus;
			bus.Send(readRequest, blocking);
		}

		public void Receive(Messages.ReadResponse readResponse)
		{
			this.FCachedRegisterValues[readResponse.RegisterType] = readResponse.Value;
			this.OnRegisterReceive?.Invoke(readResponse.RegisterType, readResponse.Value);
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

		public void Ping()
		{
			var message = new Messages.Ping(this.ID);
			var bus = this.Bus;
			bus.Send(message, false);
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
			var writeRequest = new Messages.WriteRequest(this.ID, registerType, value);
			var bus = this.Bus;
			bus.Send(writeRequest, blocking);
		}

		public void SetPrimaryRegister(int value, bool blocking)
		{
			var writeRequest = new Messages.WritePrimaryRegisterRequest(this.ID, value);
			var bus = this.Bus;
			bus.Send(writeRequest, blocking);
		}

		public override string ToString()
		{
			return String.Format("[Motor : #{0}]", this.ID);
		}
	}
}
