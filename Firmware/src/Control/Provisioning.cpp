#include "Provisioning.h"
#include "GUI/Controller.h"
#include "Devices/INA219.h"
#include "GUI/Panels/Options.h"
#include "Registry.h"

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
	Provisioning::Provisioning(Devices::MotorDriver &motorDriver
		, Devices::INA219 &ina219
		, Devices::AS5047 &as5047
		, Devices::FileSystem &fileSystem
		, EncoderCalibration &encoderCalibration
		, Interface::CANResponder &canResponder
		, Interface::WebSockets &webSockets)
	: motorDriver(motorDriver)
	, ina219(ina219)
	, as5047(as5047)
	, fileSystem(fileSystem)
	, encoderCalibration(encoderCalibration)
	, canResponder(canResponder)
	, webSockets(webSockets)
	{
		// This will happen later also, but we need it now for the "Has calibration" status
		encoderCalibration.load();
	}

	//----------
	void
	Provisioning::perform()
	{
		auto &gui = GUI::Controller::X();
		auto &registry = Registry::X();
		auto panel = std::make_shared<GUI::Panels::Options>("PROVISIONING", std::vector<GUI::Panels::Options::Option> {
			{
				[&](char * text) {
					sprintf(text, "EXIT");
				},
				[&]() {
					this->shouldExit = true;
				}
			},
			{
				[&](char * text) {
					sprintf(text, "Voltage/Current : %.1fV/%.1fA", this->status.voltage, this->status.current);
				},
				NULL
			},
			{
				[&](char * text) {
					sprintf(text, "Has calibration : %s"
						, this->encoderCalibration.getHasCalibration()
						? "TRUE"
						: "FALSE");
				},
				NULL
			},
			{
				[&](char * text) {
					sprintf(text, "Calibrate encoder");
				},
				[&]() {
					this->encoderCalibration.calibrate(this->as5047, this->motorDriver);
				}
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
					sprintf(text, "Erase disk");
				},
				[&]() {
					this->fileSystem.format();
				}
			},
			{
				[&](char * text) {
					sprintf(text, "Manual moves:");
				},
				NULL
			},
			{
				[&](char * text) {
					sprintf(text, "Step Forward");
				},
				[&]() {
					this->motorDriver.step(mod(++this->stepIndex, 4), this->settings.current);
				}
			},
			{
				[&](char * text) {
					sprintf(text, "Step Backward");
				},
				[&]() {
					this->motorDriver.step(mod(--this->stepIndex, 4), this->settings.current);
				}
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
					sprintf(text, "Provisioning enabled : %s"
						, registry.registers.at(Registry::RegisterType::ProvisioningEnabled).value
						? "TRUE"
						: "FALSE");
				},
				[&]() {
					registry.registers.at(Registry::RegisterType::ProvisioningEnabled).value ^= true;
					registry.saveDefault(Registry::RegisterType::ProvisioningEnabled);
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

			this->canResponder.update();
			this->webSockets.update();

			if(getRegisterValue(Registry::RegisterType::ProvisioningEnabled) == 0) {
				this->shouldExit = true;
			}

			gui.update();
		}

		gui.popPanel();
		vTaskDelete(task);
	}
} // namespace Control