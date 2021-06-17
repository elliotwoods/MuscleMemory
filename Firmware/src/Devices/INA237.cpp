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
		this->setCalibration();
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
		return this->currentLSB * 20.0f * (float)readValue;
	}

	//---------
	float
	INA237::getBusVoltage()
	{
		auto readValue = this->readRegister(Register::BusVoltage);

		// Overflow error
		this->errors |= readValue & 1;

		return 4e-3f * (float)(readValue >> 3);
	}

	//---------
	float
	INA237::getShuntVoltage()
	{
		auto readValue = this->readRegister(Register::ShuntVoltage);
		const auto & signedReadValue = * (int16_t*) & readValue;
		return 10e-6 * (float)(signedReadValue);
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
		this->writeRegister(Register::Configuration, 0xF000);
	}

	//---------
	void
	INA237::setConfiguration()
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
#ifdef DEBUG_CURRENT_SENSOR
		printf("Configuration set to : %#04x\n", value);
		printf("Read back : %#04x\n", this->readRegister(Register::Configuration));
#endif
	}

	//---------
	void
	INA237::setCalibration()
	{
		this->currentLSB = this->configuration.maximumCurrent / (float)(1 << 15);
		auto value = (uint16_t)((0.04096f) / (this->currentLSB * this->configuration.shuntValue));

		this->writeRegister(Register::Calibration, value);

#ifdef DEBUG_CURRENT_SENSOR
		printf("currentLSB : %f\n", this->currentLSB);
		printf("shuntValue : %f\n", this->configuration.shuntValue);
		printf("Calibration set to : %#04x\n", value);
		printf("Read back : %#04x\n", this->readRegister(Register::Calibration));
#endif
	}

	//---------
	void
	INA237::calculateGain()
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

	//---------
	void
	INA237::printDebug()
	{
		printf("Current : %f\n", this->getCurrent());
		printf("Bus voltage : %f\n", this->getBusVoltage());
		printf("Errors : %d\n", this->errors);
		printf("\n");
	}

	//---------
	void
	INA237::drawDebug(U8G2 & oled)
	{

	}
}
