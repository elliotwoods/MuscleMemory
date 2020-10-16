#include "MultiTurn.h"

#define HALF_WAY (1 << 13)
namespace Control {
	//-----------
	MultiTurn::MultiTurn(EncoderCalibration & encoderCalibration)
	: encoderCalibration(encoderCalibration)
	{
		
	}

	//-----------
	void
	MultiTurn::init(SingleTurnPosition singleTurnPosition)
	{
		this->priorSingleTurnPosition = singleTurnPosition;
		this->position = this->priorSingleTurnPosition;
		this->turns = 0;
	}

	//-----------
	void
	MultiTurn::update(SingleTurnPosition currentSingleTurnPosition)
	{
		if(this->priorSingleTurnPosition > HALF_WAY / 4 * 3 && currentSingleTurnPosition < HALF_WAY / 4)
		{
			this->turns++;
		}
		else if(this->priorSingleTurnPosition < HALF_WAY / 4 && currentSingleTurnPosition > HALF_WAY / 4 * 3)
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