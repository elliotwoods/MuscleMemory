#include "AS5047.h"

#ifdef DONT
//---------
AS5047::AS5047()
{
	spi_bus_config_t spiConfiguration {
		.miso_io_num = GPIO_NUM_19,
		.mosi_io_num = GPIO_NUM_23,
		.sclk_io_num = GPIO_NUM_18,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 32,
	};

	spi_bus_initialize(HSPI_HOST
		, &spiConfiguration
		, 0);

	spi_device_interface_config_t deviceConfiguration {
		.clock_speed_hz = 100000,
		.mode = 1,
		.spics_io_num = GPIO_NUM_5,
		.queue_size = 1
	};

	spi_bus_add_device(this->spiHostDevice
		, &deviceConfiguration);
}
#endif