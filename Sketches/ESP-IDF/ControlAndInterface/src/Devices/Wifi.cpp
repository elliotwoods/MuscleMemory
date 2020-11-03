#include "Wifi.h"

#include "../WifiConfig.h"

#include <WiFi.h>
#include <HTTPClient.h>

// Reference : https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiClient/WiFiClient.ino

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
		while(WiFi.status() != WL_CONNECTED) {
			WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
			delay(1000);
			printf("Attempting wifi connection\n");
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
		HTTPClient httpClient;

		// Format the request
		auto contentString = cJSON_PrintUnformatted(content);

		// Perform the request
		cJSON * response = nullptr;
		if(httpClient.begin((this->baseURI + path).c_str())) {
			httpClient.addHeader("Content-Type", "application/json");
			auto result = httpClient.POST((uint8_t*) contentString, strlen(contentString));
			if(result == 200) {
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
}