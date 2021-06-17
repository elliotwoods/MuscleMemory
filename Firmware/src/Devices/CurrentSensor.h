#include "Platform/Platform.h"

#ifdef MM_CONFIG_CURRENT_SENSOR_INA237
#include "INA237.h"
namespace Devices {
	typedef INA237 CurrentSensor;
}
#endif

#ifdef MM_CONFIG_CURRENT_SENSOR_INA219
#include "INA219.h"
namespace Devices {
	typedef INA219 CurrentSensor;
}
#endif

