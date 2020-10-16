#pragma once

#include "DataTypes.h"
#include "cJSON.h"
#include <string>
#include <vector>

const size_t heapAlignment = 16;
const size_t heapAreaSize = 64 * 1024;

namespace tflite {
	class Model;
	class MicroInterpreter;
}

namespace Control {
	class Agent {
	public:
		struct State {
			MultiTurnPosition position;
			MultiTurnPosition target;
			Frequency frequency;
			Velocity velocity;
			Current current;
		};
		Agent();
		~Agent();

		void init();
		float selectAction(const State &);
		void recordTrajectory(const State & priorState
			, float action
			, int32_t reward
			, const State & currentState
			);
	private:
		std::string clientID;
		std::vector<uint8_t> modelString;
		void processIncoming(cJSON *);

		const tflite::Model* model = nullptr;
		tflite::MicroInterpreter * interpreter = nullptr;
		bool initialised = false;

		uint8_t * heapArea = nullptr;
	};
}