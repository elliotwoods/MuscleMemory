#include "AS5047.h"
#include "Arduino.h"

#define SPI_HOST HSPI_HOST

#define MOSI_PIN GPIO_NUM_19
#define MISO_PIN GPIO_NUM_23
#define CLK_PIN GPIO_NUM_18

// Reference : https://github.com/espressif/esp-idf/blob/master/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c

//----------
uint16_t
AS5047::calcParity(uint16_t value)
{
	// From https://www.tutorialspoint.com/cplusplus-program-to-find-the-parity-of-a-number-efficiently#:~:text=Parity%20is%20defined%20as%20the,parity%20is%20called%20odd%20parity.
	uint16_t y = value;
	y ^= (value >> 1);
	y ^= (value >> 2);
	y ^= (value >> 4);
	y ^= (value >> 8);
	return y & 1;
}

//---------
AS5047::AS5047()
{
	// Initialise the SPI bus
	{
		spi_bus_config_t spiConfiguration;
		{
			spiConfiguration.mosi_io_num = MOSI_PIN;
			spiConfiguration.miso_io_num = MISO_PIN;
			spiConfiguration.sclk_io_num = CLK_PIN;
			spiConfiguration.quadwp_io_num = -1;
			spiConfiguration.quadhd_io_num = -1;
			spiConfiguration.max_transfer_sz = 4;
			spiConfiguration.flags = 0;
			spiConfiguration.intr_flags = 0;
		}

		auto result = spi_bus_initialize(SPI_HOST
		, &spiConfiguration
		, SPI_HOST);
		ESP_ERROR_CHECK(result);
	}


	// Add the device to the SPI bus
	{
		spi_device_interface_config_t deviceConfiguration;
		{
			deviceConfiguration.command_bits = 	0;
			deviceConfiguration.address_bits = 0;
			deviceConfiguration.dummy_bits = 0;

			deviceConfiguration.mode = 1;
			deviceConfiguration.duty_cycle_pos = 128;
			deviceConfiguration.cs_ena_pretrans = 1;
			deviceConfiguration.cs_ena_posttrans = 1;

			deviceConfiguration.clock_speed_hz = SPI_MASTER_FREQ_10M;
			deviceConfiguration.input_delay_ns = 10;
			
			deviceConfiguration.spics_io_num = GPIO_NUM_5; // Chip select
			
			deviceConfiguration.flags = 0;
			deviceConfiguration.queue_size = 7;

			deviceConfiguration.pre_cb = NULL;
			deviceConfiguration.post_cb = NULL;
		};

		auto result = spi_bus_add_device(SPI_HOST
			, &deviceConfiguration
			, &this->deviceHandle);
		ESP_ERROR_CHECK(result);
	}
}

//----------
uint16_t
AS5047::getPosition()
{
	return this->readRegister(0xFFFF);
}

//----------
AS5047::Errors
AS5047::getErrors()
{
	if(!this->hasError) {
		return Errors();
	}
	else {
		auto value = this->readRegister(0x0001);
		Errors errors;
		{
			errors.hasErrors = true;
			errors.framingError = value & 1;
			errors.invalidCommand = (value >> 1) & 1;
			errors.parityError = (value >> 2) & 1;
		}

		// clear the flag
		this->hasError = false;

		return errors;
	}
}

//----------
uint16_t
AS5047::readRegister(uint16_t request)
{
	// read
	request |= (1 << 14);

	// parity
	request |= AS5047::calcParity(request) << 15;

	uint16_t response = 0;

	spi_transaction_t transaction = {0};
	{
		transaction.length = 2;
		transaction.tx_buffer = &request;
		transaction.rx_buffer = &response;
	}

	auto result = spi_device_polling_transmit(this->deviceHandle, &transaction);
	ESP_ERROR_CHECK(result);

	return this->parseResponse(response);
}

//----------
uint16_t
AS5047::parseResponse(uint16_t response)
{
	this->hasError |= response >> 14;
	auto value = response & ((1 << 14) - 1);
	return value;
}