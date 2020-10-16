#pragma once

#include "DataTypes.h"
#include "Utils/FrameTimer.h"

extern "C" {
	#include "cJSON.h"
}

#include "FreeRTOS.h"

#include <string>
#include <vector>

const size_t localHistorySize = 256;

const size_t heapAlignment = 16;
const size_t heapAreaSize = 32 * 1024;

namespace tflite {
	class Model;
	class MicroInterpreter;
}

namespace Control {
	class Agent {
	public:
		// 192 bits. All values are rescaled to be similar magnitude ~1
		struct State {
			float position;
			float targetMinusPosition;
			float velocity;
			float agentFrequency;
			float motorControlFrequency;
			float current;
		};

		// 448 bits = 56 bytes
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
		
		void recordTrajectory(Trajectory &&);
	private:
		void processIncoming(cJSON *);

		std::string clientID;
		std::vector<uint8_t> modelString;

		Utils::FrameTimer frameTimer;

		const tflite::Model* model = nullptr;
		tflite::MicroInterpreter * interpreter = nullptr;
		bool initialised = false;

		uint8_t * heapArea = nullptr;

		Trajectory history[localHistorySize];
		size_t historyWritePosition = 0;

		State priorState;
		bool hasPriorState = false;
		float priorAction = 0.0f;
	};
}