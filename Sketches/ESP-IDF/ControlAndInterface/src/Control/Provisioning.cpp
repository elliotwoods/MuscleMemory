#include "Provisioning.h"
#include "GUI/Controller.h"
#include "Devices/INA219.h"
#include "GUI/Panels/Options.h"

int mod(int a, int b)
{
	int ret = a % b;
	if (ret < 0)
		ret += b;
	return ret;
}

void provisioningTask(void *provisioningUntyped)
{
	auto provisioning = (Control::Provisioning *)provisioningUntyped;

	while (true)
	{
		if (provisioning->settings.speed != 0)
		{
			vTaskDelay((1000 / portTICK_PERIOD_MS) / abs(provisioning->settings.speed));
			if (provisioning->settings.speed < 0)
			{
				provisioning->stepIndex--;
			}
			else
			{
				provisioning->stepIndex++;
			}
			provisioning->motorDriver.step(mod(provisioning->stepIndex, 4), provisioning->settings.current);
		}
		else
		{
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
	}
}

namespace Control
{
	//----------
	Provisioning::Provisioning(Devices::MotorDriver &motorDriver, Devices::INA219 &ina219, Devices::AS5047 &as5047)
		: motorDriver(motorDriver), ina219(ina219), as5047(as5047)
	{
	}

	//----------
	void
	Provisioning::perform()
	{
		auto &gui = GUI::Controller::X();
		auto panel = std::make_shared<GUI::Panels::Options>("PROVISIONING", std::vector<GUI::Panels::Options::Option> {
			{
				[&](char * text) {
					sprintf(text, "Voltage/Current : %.1fV/%.1fA", this->status.voltage, this->status.current);
				},
				NULL
			},
			{
				[&](char * text) {
					sprintf(text, "Current : %d", this->settings.current);
				},
				[&]() {
					this->settings.current *= 2;
					if(this->settings.current > this->settings.maxCurrent) {
						this->settings.current = 1;
					}
				}
			},
			{
				[&](char * text) {
					sprintf(text, "Speed: %d", this->settings.speed);
				},
				NULL
			},
			{
				[&](char * text) {
					sprintf(text, "Encoder : %d (%.1f%%)"
						, this->status.encoderReading
						, this->status.encoderReadingNormalised * 100.0f);
				},
				NULL
			},
			{
				[&](char * text) {
					sprintf(text, "Speed +");
				},
				[&]() {
					this->settings.speed++;
				}
			},
			{
				[&](char * text) {
					sprintf(text, "Speed -");
				},
				[&]() {
					this->settings.speed--;
				}
			},
			{
				[&](char * text) {
					sprintf(text, "EXIT");
				},
				[&]() {
					this->shouldExit = true;
				}
			},
		});
		// Set up our gui as main gui
		gui.pushPanel(panel);

		TaskHandle_t task;
		xTaskCreate(provisioningTask, "Provisioning", 2048, this, 4, &task);

		while (!this->shouldExit)
		{
			this->status.current = this->ina219.getCurrent();
			this->status.voltage = this->ina219.getBusVoltage();
			this->status.encoderReading = this->as5047.getPositionFiltered(8);
			this->status.encoderReadingNormalised = (float) this->status.encoderReading / (float) (1 << 14);
			gui.update();
		}

		gui.popPanel();
		vTaskDelete(task);
	}
} // namespace Control