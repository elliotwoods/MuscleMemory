#include "Arduino.h"

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "Dial.h"

// Rotary Encoder part
QueueHandle_t rot_enc_event_queue;
Dial dial;

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
    int16_t dialPosition = 0;

    while(1) {
        int16_t newPosition = -dial.getPosition();
        if(dialPosition!=newPosition){
            printf("Dial position : %d\n", newPosition);
            dialPosition = newPosition; 
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}


void setup()
{

    Serial.begin(115200);
    dial.init(gpio_num_t::GPIO_NUM_34, gpio_num_t::GPIO_NUM_35);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);                                // install gpio isr service
    xTaskCreate(&ROT_ENC_event, "ROT_ENC_event", 2048, NULL, 10, NULL);             // start Rotary Encoder Task
    xTaskCreate(&ROT_ENC_PUSH_event, "ROT_ENC_PUSH_event", 2048, NULL, 10, NULL);   // start Rotary Encoder Push button Task



}

void loop() {


    delay(1000);
}


