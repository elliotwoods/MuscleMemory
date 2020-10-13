#pragma once

#include "DataTypes.h"

#include "U8g2lib.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include <stdlib.h>

namespace Devices {
	class AS5047 {
		static uint16_t calcParity(uint16_t);
	public:
		enum Errors : uint8_t {
			framingError = 1,
			invalidCommand = 1 << 1,
			parityError = 1 << 2,

			errorReported = 1 << 7 // We received an error bit in the response to a request
		};

		enum class Register : uint16_t {
			None = 0
			, Errors = 0x0001
			, Diagnostics = 0x3FFC
			, CordicMagnitude = 0x3FFD
			, PositionRaw = 0x3FFE
			, PositionCompensated = 0x3FFF
		};

		void init();
		EncoderReading getPosition();
		uint8_t getErrors();
		void clearErrors();

		void printDebug();
		void drawDebug(U8G2 &);
	protected:
		uint16_t readRegister(Register);
		uint16_t parseResponse(uint16_t); ///< Returns the value and sets the local error bit
		spi_device_handle_t deviceHandle;
		bool hasIncomingError = false;
		uint8_t errors;
	};
}
