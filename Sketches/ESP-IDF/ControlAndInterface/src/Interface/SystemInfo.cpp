#include "SystemInfo.h"
#include "Registry.h"

namespace Interface {
	//----------
	SystemInfo::SystemInfo(Devices::INA219 & ina219)
	: ina219(ina219)
	{

	}

	//----------
	void
	SystemInfo::init()
	{
		this->update();
	}

	//----------
	void
	SystemInfo::update()
	{
		static auto & registry = Registry::X();
		registry.registers.at(Registry::RegisterType::FreeMemory).value = xPortGetFreeHeapSize() / 1024;
		registry.registers.at(Registry::RegisterType::Current).value = ina219.getCurrent() * 1000.0f;
		registry.registers.at(Registry::RegisterType::BusVoltage).value = ina219.getBusVoltage() * 1000.0f;
	}
}