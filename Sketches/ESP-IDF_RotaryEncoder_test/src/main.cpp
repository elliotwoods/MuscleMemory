#include "Arduino.h"

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "rotary_encoder.h"

// Rotary Encoder part
QueueHandle_t rot_enc_event_queue;
#define ROT_ENC_A_GPIO  GPIO_NUM_34
#define ROT_ENC_B_GPIO  GPIO_NUM_35
#define ENABLE_HALF_STEPS false  // Set to true to enable tracking of rotary encoder at half step resolution
#define RESET_AT          0      // Set to a positive non-zero number to reset the position if this value is exceeded
#define FLIP_DIRECTION    false  // Set to true to reverse the clockwise/counterclockwise sense

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
    for(;;) {
        if(xQueueReceive(rot_enc_push_event_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

static void ROT_ENC_event(void *pvParameter) {
    // Initialise the rotary encoder device with the GPIOs for A and B signals
    rotary_encoder_info_t info;
    rotary_encoder_init(&info, ROT_ENC_A_GPIO, ROT_ENC_B_GPIO);
    rotary_encoder_enable_half_steps(&info, ENABLE_HALF_STEPS);
    rotary_encoder_flip_direction(&info);

    // Create a queue for events from the rotary encoder driver.
    // Tasks can read from this queue to receive up to date position information.
    rot_enc_event_queue = rotary_encoder_create_queue();
    rotary_encoder_set_queue(&info, rot_enc_event_queue);

    while(1) {
        rotary_encoder_event_t event;
        if (xQueueReceive(rot_enc_event_queue, &event, 1000 / portTICK_PERIOD_MS) == pdTRUE)
        {
            printf( "Event: position %d, direction %s", event.state.position,
                     event.state.direction ? (event.state.direction == ROTARY_ENCODER_DIRECTION_CLOCKWISE ? "CW" : "CCW") : "NOT_SET");
            printf("\n");
        }
    }
}


void setup()
{

    Serial.begin(115200);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);                                // install gpio isr service
    xTaskCreate(&ROT_ENC_event, "ROT_ENC_event", 2048, NULL, 10, NULL);             // start Rotary Encoder Task
    xTaskCreate(&ROT_ENC_PUSH_event, "ROT_ENC_PUSH_event", 2048, NULL, 10, NULL);   // start Rotary Encoder Push button Task



}

void loop() {


    delay(1000);
}


