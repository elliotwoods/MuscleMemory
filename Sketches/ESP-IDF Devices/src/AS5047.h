#include <stdlib.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

class AS5047 {
public:
    AS5047();
    uint16_t getPosition();
protected:
    spi_host_device_t spiHostDevice;
};
