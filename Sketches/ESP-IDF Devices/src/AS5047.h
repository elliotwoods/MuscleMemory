#include <stdlib.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

class AS5047 {
    static uint16_t calcParity(uint16_t);
public:
    struct Errors {
        bool hasErrors = false;
        bool framingError = false;
        bool invalidCommand = false;
        bool parityError = false;
    };

    AS5047();
    uint16_t getPosition();
    Errors getErrors();
protected:
    uint16_t readRegister(uint16_t request);
    uint16_t parseResponse(uint16_t); ///< Returns the value and sets the local error bit
    spi_device_handle_t deviceHandle;
    bool hasError = false;
};
