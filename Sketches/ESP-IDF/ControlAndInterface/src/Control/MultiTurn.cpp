#include "MultiTurn.h"

#define HALF_WAY (1 << 13)
namespace Control {
	//-----------
	MultiTurn::MultiTurn(Devices::AS5047 & as5047, EncoderCalibration & encoderCalibration)
	: as5047(as5047)
	, encoderCalibration(encoderCalibration)
	{
		
	}

	//-----------
	void
	MultiTurn::init()
	{
		this->priorEncoderReading = as5047.getPosition();
		this->position = this->priorEncoderReading;
	}

	//-----------
	void
	MultiTurn::update()
	{
		auto currentEncoderReading = as5047.getPosition();
		if(this->priorEncoderReading > HALF_WAY && currentEncoderReading < HALF_WAY)
		{
			this->turns++;
		}
		else if(this->priorEncoderReading < HALF_WAY && currentEncoderReading > HALF_WAY)
		{
			this->turns--;
		}
		this->position = (((int32_t) this->turns) << 14) + (int32_t) currentEncoderReading;
		this->priorEncoderReading = currentEncoderReading;
	}

	//-----------
	int32_t
	MultiTurn::getPosition() const
	{
		return this->position;
	}
}