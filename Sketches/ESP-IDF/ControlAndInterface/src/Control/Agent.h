#pragma once

#include "DataTypes.h"
#include "cJSON.h"
#include <string>

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

		void init();
		float selectAction(const State &);
		void recordTrajectory(const State & priorState
			, float action
			, int32_t reward
			, const State & currentState
			);
	private:
		std::string clientID;
		void processIncoming(cJSON *);
		const void* model = nullptr;
		bool initialised = false;
	};
}