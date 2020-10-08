#include "I2C.h"
#include "driver/i2c.h"

//----------
I2C &
I2C::X()
{
    static I2C i2c;
    return i2c;
}

//----------
void
I2C::init()
{
    i2c_config_t busConfiguration;
	{
		busConfiguration.mode = i2c_mode_t::I2C_MODE_MASTER;
		busConfiguration.scl_io_num = GPIO_NUM_15;
		busConfiguration.sda_io_num = GPIO_NUM_4;
		busConfiguration.scl_pullup_en = gpio_pullup_t::GPIO_PULLUP_ENABLE;
		busConfiguration.sda_pullup_en = gpio_pullup_t::GPIO_PULLUP_ENABLE;
		busConfiguration.master.clk_speed = 400000;
	}
	i2c_driver_install(i2c_port_t::I2C_NUM_0, i2c_mode_t::I2C_MODE_MASTER, 0, 0, 0);
	i2c_param_config(i2c_port_t::I2C_NUM_0, &busConfiguration);
}

//----------
std::set<uint8_t>
I2C::scan()
{
    std::set<uint8_t> results;

    for (uint8_t i = 0; i < 128; i++)
    {
        auto cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        auto result = i2c_master_cmd_begin(i2c_port_t::I2C_NUM_0, cmd, 50 / portTICK_RATE_MS);

        i2c_cmd_link_delete(cmd);

        if (result == ESP_OK)
        {
            results.insert(i);
        }
    }

    return results;
}