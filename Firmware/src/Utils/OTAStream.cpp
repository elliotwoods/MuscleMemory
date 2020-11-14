#include "OTAStream.h"
#include "GUI/Controller.h"

#include "Registry.h"

namespace Utils
{
	//----------
	void
	OTAStream::begin(size_t size)
	{
		auto & gui = GUI::Controller::X();
		this->panel = std::make_shared<GUI::Panels::OTADownload>();
		gui.pushPanel(panel);
		gui.update();
		::printf("[OTAStream] Downloading (%d) bytes\n", size);

		disableCore0WDT();
		disableLoopWDT();
		this->otaPartition = esp_ota_get_next_update_partition(nullptr);
		::printf("[OTAStream] Allocating OTA download to partition (%s)\n", this->otaPartition->label);
		

		ESP_ERROR_CHECK(esp_ota_begin(this->otaPartition
			, size
			, &this->otaHandle));

		setRegisterValue(Registry::RegisterType::OTADownloading, 1);
		setRegisterValue(Registry::RegisterType::OTASize, size);
		setRegisterValue(Registry::RegisterType::OTAWritePosition, 0);
	}

	//----------
	size_t
	OTAStream::write(uint8_t data)
	{
		return this->write(&data, 1);
	}

	//----------
	size_t
	OTAStream::write(const uint8_t * data, size_t size)
	{
		auto position = getRegisterValue(Registry::RegisterType::OTAWritePosition);
		::printf("[OTAStream] Writing (%d) bytes. Received (%d) total\n", size, position);
		ESP_ERROR_CHECK(esp_ota_write(this->otaHandle
			, data
			, size));

		position += size;
		setRegisterValue(Registry::RegisterType::OTAWritePosition, position);
		GUI::Controller::X().update();
		return size;
	}

	//----------
	void
	OTAStream::end()
	{
		auto result = esp_ota_end(this->otaHandle);
		ESP_ERROR_CHECK(result);
		if(result != ESP_OK) {
			return;
		}

		// check if same as what we already ahve
		if(esp_partition_check_identity(esp_ota_get_running_partition(), this->otaPartition)) {
			::printf("[OTA] : Ignoring firmware since it is same as current\n");
			this->panel->message = "Ignore. No change.";
			GUI::Controller::X().update();
			vTaskDelay(5000 / portTICK_PERIOD_MS);
		}
		else {
			// set the boot partition to the new firmware
			ESP_ERROR_CHECK(esp_ota_set_boot_partition(this->otaPartition));

			::printf("[OTA] Firmware updated, rebooting now...\n");
			this->panel->message = "SUCCESS!";
			GUI::Controller::X().update();
			vTaskDelay(5000 / portTICK_PERIOD_MS);

			esp_restart();
		}


		GUI::Controller::X().popPanel();
	}
}