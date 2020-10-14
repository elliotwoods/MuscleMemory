#include "Dial.h"

namespace Devices
{
	//----------
	void
	Dial::init(gpio_num_t pinA, gpio_num_t pinB)
	{
		// Prepare configuration for the PCNT unit
		{

			pcnt_config_t pcnt_config;

			pcnt_config.pulse_gpio_num = pinA;
			pcnt_config.ctrl_gpio_num = pinB;
			pcnt_config.channel = PCNT_CHANNEL_0;
			pcnt_config.unit = this->pcntUnit;

			// What to do on the positive / negative edge of pulse input?
			pcnt_config.pos_mode = PCNT_COUNT_INC;   // Count up on the positive edge
			pcnt_config.neg_mode = PCNT_COUNT_DIS;   // Keep the counter value on the negative edge

			// What to do when control input is low or high?
			pcnt_config.lctrl_mode = PCNT_MODE_REVERSE; // Reverse counting direction if low
			pcnt_config.hctrl_mode = PCNT_MODE_KEEP;    // Keep the primary counter mode if high

			// Set the maximum and minimum limit values to watch
			pcnt_config.counter_h_lim = 32767;
			pcnt_config.counter_l_lim = -32768; 

			
			// Initialize PCNT unit
			pcnt_unit_config(&pcnt_config);
		}

		// Configure and enable the input filter
		pcnt_set_filter_value(this->pcntUnit, 1023);
		pcnt_filter_enable(this->pcntUnit);

		// Enable events on zero, maximum and minimum limit values
		pcnt_event_enable(this->pcntUnit, PCNT_EVT_ZERO);
		pcnt_event_enable(this->pcntUnit, PCNT_EVT_H_LIM);
		pcnt_event_enable(this->pcntUnit, PCNT_EVT_L_LIM);

		// Initialize PCNT's counter
		pcnt_counter_pause(this->pcntUnit);
		pcnt_counter_clear(this->pcntUnit);

		// Register ISR handler and enable interrupts for PCNT unit
		//pcnt_isr_register(pcnt_example_intr_handler, NULL, 0, &user_isr_handle);
		//pcnt_intr_enable(PCNT_TEST_UNIT);

		// Everything is set up, now go to counting
		pcnt_counter_resume(this->pcntUnit);
	}

	//----------
	int16_t
	Dial::getPosition()
	{
		int16_t value;
		pcnt_get_counter_value(this->pcntUnit, &value);
		return value;
	}
}
