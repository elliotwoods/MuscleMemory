#include "AS5047.h"
#include "Arduino.h"

#define SPI_HOST VSPI_HOST

#define MOSI_PIN GPIO_NUM_23
#define MISO_PIN GPIO_NUM_19
#define CLK_PIN GPIO_NUM_18

// Reference : https://github.com/espressif/esp-idf/blob/master/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c
// We're on VSPI, Phase=1, MSB first

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
void
AS5047::init()
{
	// Setup the CSn line
	// ESP_ERROR_CHECK(
	// 	gpio_set_direction(GPIO_NUM_5, gpio_mode_t::GPIO_MODE_OUTPUT)
	// );
	// ESP_ERROR_CHECK(
	// 	gpio_set_level(GPIO_NUM_5, 1)
	// );

	// Initialise the SPI bus
	{
		spi_bus_config_t busConfiguration = {0};
		{
			busConfiguration.mosi_io_num = MOSI_PIN;
			busConfiguration.miso_io_num = MISO_PIN;
			busConfiguration.sclk_io_num = CLK_PIN;
			busConfiguration.quadwp_io_num = -1;
			busConfiguration.quadhd_io_num = -1;
		}

		auto result = spi_bus_initialize(SPI_HOST
			, &busConfiguration
			, SPI_HOST);
		ESP_ERROR_CHECK(result);
	}


	// Add the device to the SPI bus
	{
		spi_device_interface_config_t deviceConfiguration = {0};
		{
			deviceConfiguration.mode = 1;

			deviceConfiguration.clock_speed_hz = 100000;
			//deviceConfiguration.input_delay_ns = 10;
			
			deviceConfiguration.queue_size = 2;

			deviceConfiguration.spics_io_num = GPIO_NUM_5;
			deviceConfiguration.cs_ena_pretrans = 2;
			deviceConfiguration.cs_ena_posttrans = 2;

			//deviceConfiguration.input_delay_ns = 50;
			//deviceConfiguration.flags = SPI_DEVICE_BIT_LSBFIRST;
		};

		auto result = spi_bus_add_device(SPI_HOST
			, &deviceConfiguration
			, &this->deviceHandle);
		ESP_ERROR_CHECK(result);
	}

	// Prepare a get position request
	{
		spi_transaction_t transaction = {0};
		{
			transaction.length = 16;
			transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
			transaction.tx_data[0] = 0xFF;
			transaction.tx_data[1] = 0xFF;
		}
		spi_device_transmit(this->deviceHandle, &transaction);
	}
}

//----------
EncoderReading
AS5047::getPosition()
{
	auto value = this->readRegister(Register::PositionCompensated);
	return value;
}

//----------
uint8_t
AS5047::getErrors()
{
	if(!this->hasIncomingError) {
		return this->errors;
	}
	else {
		this->errors |= Errors::errorReported;

		auto value = this->readRegister(Register::Errors);
		printf("Error response : %d\n", value);
		this->errors |= value;
		this->hasIncomingError = false;
		return this->errors;
	}
}

//----------
void
AS5047::clearErrors()
{
	this->errors = 0;
}

//----------
uint16_t
AS5047::readRegister(Register registerAddress)
{
	// If we're not getting position, then send request for that register
	if(registerAddress != Register::PositionCompensated) {
		// Prepare the request
		auto request = (uint16_t) registerAddress;
		{
			// read
			request |= (1 << 14);

			// parity
			request |= AS5047::calcParity(request) << 15;
		}
		
		spi_transaction_t transaction = {0};
		{
			transaction.length = 16;
			transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
			transaction.tx_data[0] = ((uint8_t*)& request)[1];
			transaction.tx_data[1] = ((uint8_t*)& request)[0];
		}
		spi_device_transmit(this->deviceHandle, &transaction);
	}

	uint16_t response = 0;

	// Perform the transaction
	{
		spi_transaction_t transaction = {0};
		{
			transaction.length = 16;
			transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
			transaction.tx_data[0] = 0xFF;
			transaction.tx_data[1] = 0xFF;
		}

		auto result = spi_device_transmit(this->deviceHandle, &transaction);
		ESP_ERROR_CHECK(result);
		
		((uint8_t*)&response)[0] = transaction.rx_data[1];
		((uint8_t*)&response)[1] = transaction.rx_data[0];
	}


	return this->parseResponse(response);
}

//----------
uint16_t
AS5047::parseResponse(uint16_t response)
{
	this->hasIncomingError |= (response >> 14) & 1;
	auto value = response & ((1 << 14) - 1);
	return value;
}

//----------
void
AS5047::printDebug()
{
	auto currentPosition = this->getPosition();
	printf("pos : %d \n", currentPosition);

	for(int i=15; i>=0; i--) {
		printf((currentPosition >> i) & 1 ? "1" : "0");
	}
	printf("\n");

	{
		auto errors = this->getErrors();
		if(errors != 0) {
			Serial.println("Some error");

			if(errors & AS5047::Errors::framingError) {
				Serial.println("Framing error");
			
			}if(errors & AS5047::Errors::invalidCommand) {
				Serial.println("Invalid command");
			}
			if(errors & AS5047::Errors::parityError) {
				Serial.println("Parity error");
			}
		}
	}
}

//----------
void
AS5047::drawDebug(U8G2 & oled)
{
	auto currentPosition = this->getPosition();

	oled.setFont(u8g2_font_fub11_tf);

	uint16_t y = 1;
	{
		char message[100];
		sprintf(message, "Enc : %d", currentPosition);
		oled.drawStr(0,y++ * 16,message);
	}

	{
		char message[100];
		sprintf(message, "Error : %d", this->getErrors());
		oled.drawStr(0, y++ * 16, message);
	}
}