#include "Wifi.h"

#include "../WifiConfig.h"

#include <WiFi.h>
#include <HTTPClient.h>


#include "GUI/Controller.h"
#include "GUI/Panels/OTADownload.h"
#include "Utils/OTAStream.h"

#include "Version.h"

// Reference : https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiClient/WiFiClient.ino

HTTPClient httpClient;

namespace Devices {
	//----------
	Wifi &
	Wifi::X()
	{
		static Wifi x;
		return x;
	}

	//----------
	void 
	Wifi::init()
	{
		printf("Connecting to : %s\n", WIFI_SSID);
		WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
		const uint16_t maxAttempts = 8;
		uint16_t attempts = 0;
		while(WiFi.status() != WL_CONNECTED) {
			attempts++;
			WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
			delay(1000);
			printf("Attempting wifi connection (%d/%d)\n", attempts, maxAttempts);

			if(attempts == maxAttempts) {
				printf("Wifi could not connect\n");
				return;
			}
		}

		printf("Wifi connected. IP Address : %d.%d.%d.%d\n"
			, WiFi.localIP()[0]
			, WiFi.localIP()[1]
			, WiFi.localIP()[2]
			, WiFi.localIP()[3]
		);

		// Render our Client ID
		{
			uint8_t macAddress[6];
			{
				auto result = esp_efuse_mac_get_default(macAddress);
				ESP_ERROR_CHECK(result);
			}
			
			char macAddressString[18];
			sprintf(macAddressString, "%X:%X:%X:%X:%X:%X", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
			this->macAddress.assign(macAddressString);
		}

		// Build the base URI
		{
			this->baseURI = "http://" MUSCLE_MEMORY_SERVER_HOST ":" MUSCLE_MEMORY_SERVER_PORT_STRING;
		}

		this->attemptOTA();
	}

	//----------
	const std::string &
	Wifi::getMacAddress() const
	{
		return this->macAddress;
	}

	//----------
	cJSON *
	Wifi::post(const std::string & path, cJSON * content)
	{
		// Format the request
		auto contentString = cJSON_PrintUnformatted(content);

		// Perform the request
		cJSON * response = nullptr;
		if(httpClient.begin((this->baseURI + path).c_str())) {
			httpClient.addHeader("Content-Type", "application/json");
			auto result = httpClient.POST((uint8_t*) contentString, strlen(contentString));
			if(result == HTTP_CODE_OK) {
				response = cJSON_Parse(httpClient.getString().c_str());
				if(!response) {
					printf("[Wifi] : Error parsing response : \n %s \n", httpClient.getString().c_str());
				}
			}
			httpClient.end();
		}

		free(contentString);

		return response;
	}

	//----------
	void
	Wifi::attemptOTA()
	{
		bool newUpdateAvailable = false;

		printf("[Wifi] Checking for OTA\n");
		{
			if(httpClient.begin((this->baseURI + "/static/Version.h").c_str())) {
				auto result = httpClient.GET();
				if(result == HTTP_CODE_OK) {
					auto text = httpClient.getString();
					char versionString[100];
					sprintf(versionString, "#define MM_VERSION \"%s\"", MM_VERSION);
					auto compare = strcmp(text.c_str(), versionString);
					if(compare == 0) {
						printf("[Wifi] Server firmware version is same as ours (%s). Ignoring\n", MM_VERSION);
					}
					else {
						printf("[Wifi] Server firmware version (%s) is different (%d) than ours (%s)\n", text.c_str(), compare, MM_VERSION);
						newUpdateAvailable = true;
					}

				}
				else {
					printf("[Wifi] Server firmware version is unknown. Ignoring. Over version is : %s\n", MM_VERSION);
				}
				httpClient.end();
			}

		}
		
		if(newUpdateAvailable) {
			printf("[Wifi] Attempting OTA\n");
			if(httpClient.begin((this->baseURI + "/static/app.bin").c_str())) {
				auto result = httpClient.GET();
				if(result == HTTP_CODE_OK) {
					printf("[WiFi] Performing OTA update\n");
					disableLoopWDT();
					disableCore0WDT();
					Utils::OTAStream otaStream;

					otaStream.begin(httpClient.getSize());

					httpClient.writeToStream(&otaStream);
					otaStream.end();
				}
				else {
					printf("[Wifi] Couldn't load OTA (%d)\n", result);
				}
				httpClient.end();
			}
		}
		
	}
}