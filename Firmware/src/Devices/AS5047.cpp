#include "AS5047.h"
#include "Arduino.h"

#define SPI_HOST VSPI_HOST

#define MOSI_PIN GPIO_NUM_23
#define MISO_PIN GPIO_NUM_19
#define CLK_PIN GPIO_NUM_18

// Reference : https://github.com/espressif/esp-idf/blob/master/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c
// We're on VSPI, Phase=1, MSB first

namespace Devices {
	//---------
	void
	AS5047::init()
	{
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

				deviceConfiguration.clock_speed_hz = SPI_MASTER_FREQ_16M;
				//deviceConfiguration.input_delay_ns = 10;
				
				deviceConfiguration.queue_size = 2;

				deviceConfiguration.spics_io_num = GPIO_NUM_5;
				deviceConfiguration.cs_ena_pretrans = 2;
				deviceConfiguration.cs_ena_posttrans = 2;

				deviceConfiguration.input_delay_ns = 51;
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
	EncoderReading IRAM_ATTR
	AS5047::getPosition()
	{
		auto value = this->readRegister(Register::PositionCompensated);
		return value;
	}

	//----------
	EncoderReading IRAM_ATTR
	AS5047::getPositionFiltered(uint8_t windowSize)
	{
		const auto halfWay = (EncoderReading) (1 << 13);
		const auto firstQuarter = halfWay * 1 / 2;
		const auto lastQuarter = halfWay * 3 / 2;

		std::vector<EncoderReading> readings;

		// Collect samples
		for(uint8_t i=0; i<windowSize; i++) {
			auto currentReading = this->readRegister(Register::PositionCompensated);
			if(readings.empty()) {
				readings.push_back(currentReading);
			}
			else {
				// Consider that we might have cycled
				const auto & priorReading = readings.back();
				if((priorReading > lastQuarter && currentReading < firstQuarter)
					|| (priorReading < firstQuarter && currentReading > lastQuarter)) {
					// We cycled
					readings.clear();
				}
				readings.push_back(currentReading);
			}
		}

		if(readings.size() == 0) {
			return 0;
		}
		else if(readings.size() == 1) {
			return readings.front();
		}
		else if(readings.size() == 2) {
			auto sum = (uint32_t) readings.front() + (uint32_t) readings.back();
			return (EncoderReading) (sum / 2);
		}
		else {
			// Return the mean of the 25th to 75th percentiles
			std::sort(readings.begin(), readings.end());
			uint32_t accumulator = 0;
			size_t firstIndex = readings.size() * 1 / 4;
			size_t lastIndex = readings.size() * 3 / 4;
			for(size_t i=firstIndex; i<lastIndex; i++) {
				accumulator += readings[i];
			}
			return (EncoderReading) (accumulator / (lastIndex - firstIndex));
		}
		
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
	AS5047::Diagnostics
	AS5047::getDiagnostics()
	{
		auto value = this->readRegister(Register::Diagnostics);
		Diagnostics diagnostics;
		{
			diagnostics.fieldStrengthTooLow = (value >> 11) & 1;
			diagnostics.fieldStrengthTooHigh = (value >> 10) & 1;
			diagnostics.cordicOverflow = (value >> 9) & 1;
			diagnostics.internalOffsetLoopFinished = (value >> 8) & 1;
			diagnostics.automaticGainControl = value & 0xFF;
		}
		return diagnostics;
	}

	//----------
	uint16_t
	AS5047::getCordicMagnitude()
	{
		return this->readRegister(Register::CordicMagnitude);
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
				request |= __builtin_parity(request) << 15;
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
}
