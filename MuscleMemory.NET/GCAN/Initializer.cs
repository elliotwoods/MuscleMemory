using System;
using System.Collections.Generic;
using System.Text;

namespace GCAN
{
	public class Initializer
	{
		static public void RegisterDevices()
		{
			// We only support one device type here
			UInt32 deviceType = 4;

			UInt32 deviceIndex = 0;
			while(NativeFunctions.OpenDevice(deviceType, deviceIndex, 0) == NativeFunctions.ECANStatus.STATUS_OK)
			{
				NativeFunctions.CloseDevice(deviceType, deviceIndex);
				Candle.Device.RegisterAlternativeDevice(new Device(deviceType, deviceIndex));
				deviceIndex++;
			}
		}
	}
}
