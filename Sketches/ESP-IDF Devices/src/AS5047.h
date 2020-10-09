#include <stdlib.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "U8g2lib.h"

class AS5047 {
    static uint16_t calcParity(uint16_t);
public:
    enum Errors : uint8_t {
        framingError = 1,
        invalidCommand = 1 << 1,
        parityError = 1 << 2,

        errorReported = 1 << 7 // We received an error bit in the response to a request
    };

    AS5047();
    uint16_t getPosition();
    uint8_t getErrors();
    void clearErrors();

    void printDebug();
    void drawDebug(U8G2 &);
protected:
    uint16_t readRegister(uint16_t request);
    uint16_t parseResponse(uint16_t); ///< Returns the value and sets the local error bit
    spi_device_handle_t deviceHandle;
    bool hasIncomingError = false;
    uint8_t errors;
};
