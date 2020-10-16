#include "Agent.h"

#include "stdint.h"
#include "../Devices/Wifi.h"
#include "esp_err.h"
#include "esp_system.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "crypto/base64.h"

namespace Control {
	//----------
	void
	Agent::init()
	{
		// Render our Client ID
		{
			uint8_t macAddress[6];
			{
				auto result = esp_efuse_mac_get_default(macAddress);
				ESP_ERROR_CHECK(result);
			}
			
			char clientID[18];
			sprintf(clientID, "%X:%X:%X:%X:%X:%X", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
			this->clientID = std::string(clientID);
		}

		// Start the session on the server
		{
			// Make the request
			cJSON * response = nullptr;
			{
				auto request = cJSON_CreateObject();
				cJSON_AddStringToObject(request, "client_id", this->clientID.c_str());
				response = Devices::Wifi::X().post("startSession", request);
				cJSON_Delete(request);
			}
			
			if(!response) {
				this->initialised = false;
			}
			else {
				this->processIncoming(response);
				cJSON_Delete(response);
			}
		}
	}

	//----------
	float
	Agent::selectAction(const State & state)
	{
		if(!this->initialised) {
			printf("[Agent] : Cannot infer action (not initialised)\n");
			return 0.0f;
		}

		return 1.0f / 16.0f;
	}

	//----------
	void
	Agent::recordTrajectory(const State & priorState
		, float action
		, int32_t reward
		, const State & currentState)
	{
		if(!this->initialised) {
			printf("[Agent] : Cannot record trajectory (not initialised)\n");
			return;
		}
	}

	//----------
	void
	Agent::processIncoming(cJSON * response)
	{
		auto responseString = cJSON_Print(response);
		printf("Response : %s\n", responseString);
		free(responseString);
		this->initialised = true;
	}
}