#include <set>
#include "stdint.h"

class I2C {
public:
    static I2C & X();
    void init();
    std::set<uint8_t> scan();
};
