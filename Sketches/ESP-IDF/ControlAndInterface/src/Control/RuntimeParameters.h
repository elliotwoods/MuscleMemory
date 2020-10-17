#pragma once

#include "cJSON.h"

namespace Control {
	class RuntimeParameters {
	public:
		void deserialize(cJSON *);

		bool isTraining;
		float noiseAmplitude;
	};
}