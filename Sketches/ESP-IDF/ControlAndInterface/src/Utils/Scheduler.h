#pragma once

#include "driver/timer.h"
#include <vector>

namespace Utils {
	class Scheduler {
	public:
		struct TimerSlot {
			timer_group_t group;
			timer_idx_t index;
		};

		const std::vector<TimerSlot> timerSlots {
			{ TIMER_GROUP_0, TIMER_0}
			, { TIMER_GROUP_0, TIMER_1}
			, { TIMER_GROUP_1, TIMER_0}
			, { TIMER_GROUP_1, TIMER_1}
		};

		struct TimerTask {
			const TimerSlot * timerSlot;	
			void(*callback)(void);
		};

	private:
		Scheduler() {}
	public:
		static Scheduler & X();
		void schedule(float intervalSeconds, void(*callback)(void));
	private:
		std::vector<TimerTask> timerTasks;
	};
}