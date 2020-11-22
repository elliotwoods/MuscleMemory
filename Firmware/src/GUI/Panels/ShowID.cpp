#include "ShowID.h"
#include "Registry.h"
#include "GUI/Controller.h"

namespace GUI {
	namespace Panels {
		void ShowID::show()
		{
			auto panel = std::make_shared<GUI::Panels::ShowID>();
			GUI::Controller::X().pushPanel(panel);
			while(!panel->shouldExit) {
				GUI::Controller::X().update();
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			GUI::Controller::X().popPanel();
		}

		//----------
		ShowID::ShowID()
		{
			this->startTime = esp_timer_get_time();
		}

		//----------
		void
		ShowID::update()
		{
			auto timeShown = esp_timer_get_time() - this->startTime;
			if(!editing && timeShown > 1000000) {
				this->shouldExit = true;
			}
		}

		//----------
		void
		ShowID::draw(U8G2& u8g2)
		{
			const auto & value = getRegisterValue(Registry::RegisterType::DeviceID);
			
			char message[100];
			
			u8g2.setFont(u8g2_font_nerhoe_tr ); 
			sprintf(message, "Device ID [0x%X]", value);
			u8g2.drawStr(3, 12, message);	

			u8g2.setFont(u8g2_font_inb38_mr);
			sprintf(message, "%d", value);
			int IDxPosition = 93;
			auto pxWidth = int(log(value) / log(10)) * 31;

			u8g2.drawStr(IDxPosition - pxWidth, 53, message);

			if(this->editing) {
				u8g2.drawFrame(0, 0, 128, 64);
			}
		}

		
		//----------
		void
		ShowID::buttonPressed()
		{
			if(!this->editing) {
				this->editing = true;
			}
			else {
				Registry::X().saveDefault(Registry::RegisterType::DeviceID);
				this->editing = false;
				this->shouldExit = true;

				// For the time being - always reboot since we can't restart CAN safely
				esp_restart();
			}
		}

		
		//----------
		void
		ShowID::dial(int8_t movements)
		{
			if(this->editing) {
				auto deviceID = getRegisterValue(Registry::RegisterType::DeviceID);
				const auto & range = getRegisterRange(Registry::RegisterType::DeviceID);

				deviceID += movements;
				if(deviceID < range.min) {
					deviceID = range.max;
				}
				else if(deviceID > range.max) {
					deviceID = range.min;
				}
				setRegisterValue(Registry::RegisterType::DeviceID, deviceID);
			}
		}
	}
}