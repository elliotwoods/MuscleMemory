#pragma once

#include "DataTypes.h"
#include "Utils/FrameTimer.h"
#include "OUActionNoise.h"
#include "RuntimeParameters.h"

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

		void loadModel(const char *, size_t size);
		void unloadModel();

		void serverCommunicateTask();

		float selectAction(const State &);
		
		void recordTrajectory(Trajectory &&);
	private:
		bool checkInputSize();
		bool checkOutputSize();
		void processIncoming(cJSON *);


		std::string clientID;
		std::vector<uint8_t> modelString;

		Utils::FrameTimer frameTimer;

		const tflite::Model* model = nullptr;
		tflite::MicroInterpreter * interpreter = nullptr;
		bool initialised = false;

		uint8_t * heapArea = nullptr;

		struct History {
			Trajectory trajectories[localHistorySize];
			size_t writePosition = 0;
		};
		History * historyWrites;
		History * historyReads;
		SemaphoreHandle_t historyMutex;
		QueueHandle_t historyToServer;
		

		State priorState;
		bool hasPriorState = false;
		float priorAction = 0.0f;

		OUActionNoise actionNoise;

		RuntimeParameters runtimeParameters;
	};
}