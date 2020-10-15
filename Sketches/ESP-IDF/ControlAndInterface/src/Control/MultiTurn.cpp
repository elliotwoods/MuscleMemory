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
		this->priorSingleTurnPosition = as5047.getPosition();
		this->position = this->priorSingleTurnPosition;
	}

	//-----------
	void
	MultiTurn::update(PositionWithinShaftCycle currentSingleTurnPosition)
	{
		if(this->priorSingleTurnPosition > HALF_WAY && currentSingleTurnPosition < HALF_WAY)
		{
			this->turns++;
		}
		else if(this->priorSingleTurnPosition < HALF_WAY && currentSingleTurnPosition > HALF_WAY)
		{
			this->turns--;
		}
		this->position = (((int32_t) this->turns) << 14) + (int32_t) currentSingleTurnPosition;
		this->priorSingleTurnPosition = currentSingleTurnPosition;
	}

	//-----------
	int32_t
	MultiTurn::getMultiTurnPosition() const
	{
		return this->position;
	}
}