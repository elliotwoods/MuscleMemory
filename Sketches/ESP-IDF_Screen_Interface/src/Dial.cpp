//#include "rotary_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "GuiController.h"
#include "Dial.h"

// Rotary Encoder part ------------------------------------------------------
QueueHandle_t rot_enc_event_queue;


// Rotary Encoder Push button part
#define ROT_ENC_PUSH_GPIO  GPIO_NUM_39
#define GPIO_INPUT_PIN_SEL  (1ULL<<ROT_ENC_PUSH_GPIO)
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle rot_enc_push_event_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(rot_enc_push_event_queue, &gpio_num, NULL);
}

static void ROT_ENC_PUSH_event(void* arg)
{
    // Button pin of Rotary Encoder
    gpio_config_t ROT_ENC_push_io_conf;

    ROT_ENC_push_io_conf.intr_type = GPIO_INTR_ANYEDGE;          //interrupt of rising edge
    ROT_ENC_push_io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;      //bit mask of the pins, use GPIO4/5 here
    ROT_ENC_push_io_conf.mode = GPIO_MODE_INPUT;                 //set as input mode 
    ROT_ENC_push_io_conf.pull_up_en = GPIO_PULLUP_ENABLE;        //enable pull-up mode
    gpio_config(&ROT_ENC_push_io_conf);
    
    rot_enc_push_event_queue = xQueueCreate(10, sizeof(uint32_t));    //create a queue to handle gpio event from isr 

    gpio_isr_handler_add(ROT_ENC_PUSH_GPIO, gpio_isr_handler, (void*) ROT_ENC_PUSH_GPIO);       //hook isr handler for specific gpio pin

    gpio_num_t io_num;
	int xLevel = 1;
    for(;;) {
        if(xQueueReceive(rot_enc_push_event_queue, &io_num, portMAX_DELAY)) {
			if(gpio_get_level(io_num)!=xLevel){
				xLevel = gpio_get_level(io_num);
				if(xLevel==0) {
                    GuiController::X().dialButtonPressed();
                }
			}
        }
    }
}

//----------
Dial & 
Dial::X()
{
	static Dial d;
	return d;
}

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

static void ROT_ENC_event(void *pvParameter) {
    int16_t previousPosition = 0;
    while(1) {
        int16_t position = -Dial::X().getPosition();
        if(previousPosition!=position){
            GuiController::X().dialTurned(position - previousPosition);
            //printf("pos %d, prev pos %d \n", position, previousPosition);
            previousPosition = position;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void initDial() {
    
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);                                // install gpio isr service
    xTaskCreate(&ROT_ENC_event, "ROT_ENC_event", 2048, NULL, 10, NULL);             // start Rotary Encoder Task
    xTaskCreate(&ROT_ENC_PUSH_event, "ROT_ENC_PUSH_event", 2048, NULL, 10, NULL);   // start Rotary Encoder Push button Task
}