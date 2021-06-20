#include "INA237.h"
#include "I2C.h"

//#define DEBUG_CURRENT_SENSOR

namespace Devices {
	//---------
	INA237::INA237()
	{
	}

	//---------
	void
	INA237::init(const INA237::Configuration &configuration)
	{
		this->configuration = configuration;

		this->reset();
		this->setConfiguration();
		this->setADCConfiguration();
		this->setShuntCalibration();
	}

	//---------
	float
	INA237::getCurrent()
	{
		auto readValue = this->readRegister(Register::Current);
		auto &signedValue = *(int16_t *)&readValue;
		return this->currentLSB * (float)signedValue;
	}

	//---------
	float
	INA237::getPower()
	{
		auto readValue = this->readRegister(Register::Power);
		return this->currentLSB * 3.2f * (float)readValue;
	}

	//---------
	float
	INA237::getBusVoltage()
	{
		auto readValue = this->readRegister(Register::BusVoltage);
		auto &signedValue = *(int16_t *)&readValue;
		return 3.125e-3f * (float)signedValue;
	}

	//---------
	float
	INA237::getShuntVoltage()
	{
		auto readValue = this->readRegister(Register::ShuntVoltage);
		const auto & signedReadValue = * (int16_t*) & readValue;
		return this->configuration.shuntRange == Configuration::ShuntRange::ShuntRange_163_84mV
			? 5e-6 * (float) (signedReadValue)
			: 1.25e-6 * (float) (signedReadValue);
	}

	//---------
	float
	INA237::getTemperature()
	{
		auto readValue = this->readRegister(Register::Temperature);
		const auto & signedReadValue = * (int16_t*) & readValue;
		return (125e-3) * (float) (signedReadValue >> 3);
	}

	//---------
	uint16_t
	INA237::readRegister(Register registerAddress)
	{
		{
			auto cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			{
				i2c_master_write_byte(cmd, (this->configuration.address << 1) | I2C_MASTER_WRITE, true);
				i2c_master_write_byte(cmd, (uint8_t) registerAddress, true);
			}
			i2c_master_stop(cmd);
			I2C::X().perform(cmd);
		}

		uint16_t value;
		auto data = (uint8_t *) &value;
		{
			auto cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			{
				i2c_master_write_byte(cmd, (this->configuration.address << 1) | I2C_MASTER_READ, true);
				i2c_master_read_byte(cmd, data + 1, i2c_ack_type_t::I2C_MASTER_ACK);
				i2c_master_read_byte(cmd, data, i2c_ack_type_t::I2C_MASTER_ACK);
			}
			i2c_master_stop(cmd);
			I2C::X().perform(cmd);
		}

		return value;
	}

	//---------
	void
	INA237::writeRegister(Register registerAddress, uint16_t value)
	{
		auto data = (uint8_t*) &value;

		auto cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		{
			i2c_master_write_byte(cmd, (this->configuration.address << 1) | I2C_MASTER_WRITE, true);
			i2c_master_write_byte(cmd, (uint8_t) registerAddress, true);
			i2c_master_write_byte(cmd, data[1], true);
			i2c_master_write_byte(cmd, data[0], true);
		}
		i2c_master_stop(cmd);

		I2C::X().perform(cmd);
	}

	//---------
	void
	INA237::reset()
	{
		// Set the reset bit
		this->writeRegister(Register::Configuration, 0xF000);
	}

	//---------
	void
	INA237::setConfiguration()
	{
		uint16_t value = 0;
		value |= (uint16_t) this->configuration.conversionDelay << 6;
		value |= (uint16_t) this->configuration.shuntRange << 4;
		this->writeRegister(Register::Configuration, value);

#ifdef DEBUG_CURRENT_SENSOR
		printf("Configuration set to : %#04x\n", value);
		printf("Read back : %#04x\n", this->readRegister(Register::Configuration));
#endif
	}

	//---------
	void
	INA237::setADCConfiguration()
	{
		uint16_t value = 0;
		value |= (uint16_t) this->configuration.operatingMode << 12;
		value |= (uint16_t) this->configuration.busVoltageConversionTime << 9;
		value |= (uint16_t) this->configuration.shuntVoltageConversionTime << 6;
		value |= (uint16_t) this->configuration.temperatureConversionTime << 3;
		value |= (uint16_t) this->configuration.sampleCount;
		
		this->writeRegister(Register::ADCConfiguration, value);

#ifdef DEBUG_CURRENT_SENSOR
		printf("ADC Configuration set to : %#04x\n", value);
		printf("Read back : %#04x\n", this->readRegister(Register::ADCConfiguration));
#endif
	}

	//---------
	void
	INA237::setShuntCalibration()
	{
		this->currentLSB = this->configuration.maximumCurrent / (float) pow(2, 15);
		float currLSBCalc = (float) 12107.2e6 * this->currentLSB * this->configuration.shuntValue;

		auto  value = (uint16_t) currLSBCalc;
		this->writeRegister(Register::ShuntCalibration, value);

#ifdef DEBUG_CURRENT_SENSOR
		printf("Shunt calibration  set to : %#04x\n", value);
		printf("Read back : %#04x\n", this->readRegister(Register::ShuntCalibration));
#endif
	}

	//---------
	void
	INA237::printDebug()
	{
		printf("Current : %f\n", this->getCurrent());
		printf("Bus voltage : %f\n", this->getBusVoltage());
		printf("Temperature : %f\n", this->getTemperature());
		printf("Errors : %d\n", this->errors);
		printf("\n");
	}

	//---------
	void
	INA237::drawDebug(U8G2 & oled)
	{

	}
}
