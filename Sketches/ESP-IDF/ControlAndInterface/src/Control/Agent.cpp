#include "Agent.h"

#include "stdint.h"
#include "../Devices/Wifi.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

extern "C" {
	#include "crypto/base64.h"
}

tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter* error_reporter = &micro_error_reporter;

tflite::AllOpsResolver resolver;

namespace Control {
	//----------
	Agent::Agent()
	{
		this->heapArea = (uint8_t*) heap_caps_malloc(heapAreaSize + heapAlignment, MALLOC_CAP_8BIT);
	}

	//----------
	Agent::~Agent()
	{
		heap_caps_free(this->heapArea);
		this->heapArea = nullptr;
	}

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
	// Reference : https://github.com/elliotwoods/micropython/blob/elliot-modules/modules/tensorflow/Model.cpp
	void
	Agent::processIncoming(cJSON * response)
	{
		// Print response
		{
			auto responseString = cJSON_Print(response);
			printf("Response : %s\n", responseString);
			free(responseString);
		}
		

		// Check success
		{
			auto successJson = cJSON_GetObjectItemCaseSensitive(response, "success");
			if(!successJson 
				|| !cJSON_IsBool(successJson)
				|| !successJson->valueint) {
				printf("[Agent] : Server request failed\n");
				return;
			}
		}

		// Extract model
		{
			auto contentJson = cJSON_GetObjectItemCaseSensitive(response, "content");
			if(contentJson && cJSON_IsObject(contentJson)) {
				auto modelJson = cJSON_GetObjectItemCaseSensitive(contentJson, "model");
				if(modelJson && cJSON_IsString(modelJson) && modelJson->valuestring) {
					// Decode from BASE64 into local copy
					{
						size_t outputLength;
						auto binaryString = base64_decode((const unsigned char *) modelJson->valuestring
							, strlen(modelJson->valuestring)
							, &outputLength);
						this->modelString.assign(binaryString, binaryString + outputLength);
						free(binaryString);
					}

					// Load the model
					{
						this->model = ::tflite::GetModel(this->modelString.data());
						if(this->model->version() != TFLITE_SCHEMA_VERSION) {
							TF_LITE_REPORT_ERROR(error_reporter,
								"Model provided is schema version %d not equal "
								"to supported version %d.\n",
								this->model->version(), TFLITE_SCHEMA_VERSION);
							
							return;
						}
					}
					
					// Initialise the interpreter
					{
						// Align the memory (note this only works for 16bit)
						auto heapAreaAligned = ((uintptr_t)this->heapArea + 15) & ~ (uintptr_t) 0x0F;
						this->interpreter = new tflite::MicroInterpreter(model
							, resolver
							, (uint8_t*) heapAreaAligned
							, heapAreaSize
							, error_reporter);
					}
					
					
					this->initialised = true;
				}
			}
		}
		
	}
}