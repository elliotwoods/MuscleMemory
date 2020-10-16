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
	Wifi::init(const std::string & baseURI)
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
			, WiFi.localIP()[3]
			, WiFi.localIP()[4]
		);

		{
			this->baseURI = baseURI;

			//add trailing slash if needed
			if(this->baseURI.back() != '/') {
				this->baseURI.push_back('/');
			}
		}
	}

	//----------
	cJSON *
	Wifi::post(const std::string & path, cJSON * content)
	{
		HTTPClient httpClient;

		// Format the request
		auto contentString = cJSON_Print(content);

		// Perform the request
		cJSON * response = nullptr;
		if(httpClient.begin((this->baseURI + path).c_str())) {
			httpClient.addHeader("Content-Type", "application/json");
			auto result = httpClient.POST((uint8_t*) contentString, strlen(contentString));
			if(result == 200) {
				response = cJSON_Parse(httpClient.getString().c_str());
			}
			httpClient.end();
		}

		free(contentString);

		return response;
	}
}