#include "Scheduler.h"

#include "esp_timer.h"
#include "esp_types.h"
#include "driver/timer.h"

#ifdef SCHEDULER_ENABLE

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to s


namespace Utils {
	//----------
	void
	isr(void * timerTaskUntyped)
	{
		printf("isr\n");
		auto timerTask = (Scheduler::TimerTask*) timerTaskUntyped;
		
		auto & timer = TIMERG0.hw_timer[timerTask->timerSlot->index];
		timer.update = 1;
		TIMERG0.int_clr_timers.t0 = 1;
		
		TIMERG0.hw_timer[timerTask->timerSlot->index].update = 1;


		timerTask->callback();
	}

	//----------
	Scheduler &
	Scheduler::X()
	{
		static Scheduler x;
		return x;
	}

	//----------
	void
	Scheduler::schedule(float intervalSeconds, void(*callback)(void))
	{
		const auto & timerSlot = timerSlots[this->timerTasks.size()];
		this->timerTasks.push_back({
			&timerSlot
			, callback
		});

		timer_config_t config = {0};
		{
			config.divider = TIMER_DIVIDER;
			config.counter_dir = TIMER_COUNT_UP;
			config.counter_en = TIMER_PAUSE;
			config.alarm_en = TIMER_ALARM_EN;
			config.auto_reload = true;
		}

		timer_init(timerSlot.group, timerSlot.index, &config);

		timer_set_counter_value(timerSlot.group, timerSlot.index, 0ULL);
		timer_set_alarm_value(timerSlot.group, timerSlot.index, intervalSeconds * float(TIMER_SCALE));
		timer_enable_intr(timerSlot.group, timerSlot.index);
		timer_isr_register(timerSlot.group
			, timerSlot.index
			, isr
			, (void*) &this->timerTasks.back()
			, ESP_INTR_FLAG_IRAM
			, NULL);
		timer_start(timerSlot.group, timerSlot.index);
	}
}

#endif
