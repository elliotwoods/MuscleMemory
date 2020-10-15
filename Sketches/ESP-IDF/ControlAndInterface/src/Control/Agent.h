#pragma once
namespace Control {
	class Agent {
	public:
		struct State {
			float position;
			float velocity;
			float target;
			float current;
		};

		void init();
		float selectAction(const State &);
		void recordTrajectory(const State & priorState
			, float action
			, const State & currentState
			, float reward);
	private:
		const void* model = nullptr;
	};
}