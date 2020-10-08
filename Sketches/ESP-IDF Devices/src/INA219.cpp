#include "INA219.h"

//---------
INA219::INA219()
{
}

//---------
void
INA219::init(const INA219::Configuration &configuration, uint8_t address)
{
	this->configuration = configuration;

	this->reset();
	this->setConfiguration();
	this->setCalibration();


}

//---------
float
INA219::getCurrent()
{
	auto readValue = this->readRegister(Register::Current);
	auto &signedValue = *(int16_t *)&readValue;
	return this->lsbToCurrent * (float)signedValue;
}

//---------
float
INA219::getPower()
{
	auto readValue = this->readRegister(Register::Power);
	return this->lsbToCurrent * 20 * (float)readValue;
}

//---------
float
INA219::getBusVoltage()
{
	auto readValue = this->readRegister(Register::BusVoltage);

	// Overflow error
	this->errors |= readValue & 1;

	return 4e-3f * (float)(readValue >> 3);
}

//---------
uint16_t
INA219::readRegister(Register register)
{
	return 0;
}

//---------
void
INA219::writeRegister(Register register, uint16_t value)
{
}

//---------
void
INA219::reset()
{
	this->writeRegister(Register::Configuration, 1 << 15);
}

//---------
void
INA219::setConfiguration()
{
	if (this->configuration.gain == Configuration::Gain::Gain_Auto)
	{
		this->calculateGain();
	}

	uint16_t value = 0;
	{
		value |= this->configuration.operatingMode;
		value |= this->configuration.currentResolution << 3;
		value |= this->configuration.busVoltageResolution << 7;
		value |= this->configuration.gain << 11;
		value |= this->configuration.voltageRange << 13;
	}

	this->writeRegister(Register::Configuration, value);
}

//---------
void
INA219::setCalibration()
{
	auto current_lsb = this->configuration.maximumCurrent / (float)(1 << 15);
	auto value = (uint16_t)((0.04096f) / (current_lsb * this->configuration.shuntValue));

	this->writeRegister(Register::Calibration, value);
	this->lsbToCurrent = current_lsb;
}

//---------
void
INA219::calculateGain()
{
	auto maximumShuntVoltage = this->configuration.maximumCurrent * this->configuration.shuntValue;
	if (maximumShuntVoltage < 40e-3)
	{
		this->configuration.gain = Configuration::Gain_1_Range_40mV;
	}
	else if (maximumShuntVoltage < 80e-3)
	{
		this->configuration.gain = Configuration::Gain_2_Range_80mV;
	}
	else if (maximumShuntVoltage <= 160e-3)
	{
		this->configuration.gain = Configuration::Gain_4_Range_160mV;
	}
	else
	{
		this->configuration.gain = Configuration::Gain_8_Range_320mV;
	}
}