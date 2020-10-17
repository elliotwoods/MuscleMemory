#include "OUActionNoise.h"

#include <math.h>
#include <cstdlib>

namespace Control {
	//----------
	void
	OUActionNoise::init(float mean, float standardDeviation, float theta, float dt)
	{
		this->mean = mean;
		this->standardDeviation = standardDeviation;
		this->theta = theta;
		this->dt = dt;

		this->reset();
	}
	
	//----------
	void
	OUActionNoise::reset()
	{
		this->previousValue = 0.0f;
	}

	//----------
	float
	OUActionNoise::get()
	{
		auto random = float(rand()) / float(RAND_MAX) - 0.5f;
		float value = this->previousValue
			+ this->theta * (this->mean - this->previousValue) * this->dt
			+ this->standardDeviation * sqrt(this->dt) * random;
		this->previousValue = value;
		return value;
	}
}