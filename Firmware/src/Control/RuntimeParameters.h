#pragma once

#include "cJSON.h"

namespace Control {
	class RuntimeParameters {
	public:
		void deserialize(cJSON *);

		bool isTraining = false;
		float noiseAmplitude = 0.0f;
		float addProportional = 0.0f;
		float addConstant = 0.0f;
	};
}