#include "Platform/Platform.h"

#if defined(ARDUINO) && defined(ARDUINO_OLD_STYLE)
    #include "FreeRTOS.h"
#else
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/queue.h>
    #include <freertos/semphr.h>
#endif