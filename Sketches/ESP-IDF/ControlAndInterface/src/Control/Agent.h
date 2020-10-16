#pragma once

#include "DataTypes.h"

extern "C" {
	#include "cJSON.h"
}

#include "FreeRTOS.h"

#include <string>
#include <vector>

const size_t localHistorySize = 256;
const size_t trajectoryQueueSize = 128;

const size_t heapAlignment = 16;
const size_t heapAreaSize = 64 * 1024;

namespace tflite {
	class Model;
	class MicroInterpreter;
}

namespace Control {
	class Agent {
	public:
		// 128 bits
		struct State {
			MultiTurnPosition position;
			MultiTurnPosition targetMinusPosition;
			Frequency frequency;
			Velocity velocity;
			Current current;
		};

		struct Trajectory {
			State priorState;
			float action;
			float reward;
			State currentState;
		};

		Agent();
		~Agent();

		void init();
		void update();
		
		float selectAction(const State &);
		void recordTrajectory(const Trajectory &);
	private:
		std::string clientID;
		std::vector<uint8_t> modelString;
		void processIncoming(cJSON *);

		const tflite::Model* model = nullptr;
		tflite::MicroInterpreter * interpreter = nullptr;
		bool initialised = false;

		uint8_t * heapArea = nullptr;

		QueueHandle_t trajectoryQueue;
		Trajectory history[localHistorySize];
		size_t historyWritePosition = 0;
	};
}