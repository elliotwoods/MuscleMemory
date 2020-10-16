#include "Registry.h"

//----------
Registry& Registry::X() {
    static Registry registry;
    return registry;
}