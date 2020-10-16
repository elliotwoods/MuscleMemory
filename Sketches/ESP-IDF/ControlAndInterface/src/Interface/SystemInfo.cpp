#include "SystemInfo.h"
#include "Registry.h"

#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

int temperature_sens_read() {
	SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3, SENS_FORCE_XPD_SAR_S);
	SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_DIV, 10, SENS_TSENS_CLK_DIV_S);
	CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
	CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
	SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP_FORCE);
	SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
	ets_delay_us(100);
	SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
	ets_delay_us(5);
	return GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S);
}

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
		registry.registers.at(Registry::RegisterType::Temperature).value = float(temperature_sens_read() - 32) / 1.8f;
	}
}