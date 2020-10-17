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

#include "Registry.h"

extern "C" {
	#include "crypto/base64.h"
}

tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter* error_reporter = &micro_error_reporter;

tflite::AllOpsResolver resolver;

int8_t min(int8_t a, int8_t b) {
	if(a > b) {
		return b;
	}
	else {
		return a;
	}
}

int8_t max(int8_t a, int8_t b) {
	if(a > b) {
		return a;
	}
	else {
		return b;
	}
}

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

		// Intialise the frame timer
		this->frameTimer.init();
	}

	//----------
	void
	Agent::update()
	{
		static auto & registry = Registry::X();
		this->frameTimer.update();

		// Read the state from registry
		Registry::AgentReads agentReads;
		{
			registry.agentRead(agentReads);
		}

		// Prepare the state
		Agent::State state;
		{
			state.position = float(agentReads.multiTurnPosition) / float(1 << 14);
			state.targetMinusPosition = float(agentReads.targetPosition - agentReads.multiTurnPosition) / float(1 << 14);
			state.velocity = float(agentReads.velocity) / float(1 << 14);
			state.agentFrequency = float(this->frameTimer.getFrequency()) / 1000.0f;
			state.motorControlFrequency = float(agentReads.motorControlFrequency) / 1000.0f;
			state.current = float(agentReads.current) / 1000.0f;
		}

		// Record trajectory if we have a prior state
		if(this->hasPriorState) {
			int32_t reward = abs(state.targetMinusPosition);
			this->recordTrajectory({
				this->priorState
				, this->priorAction
				, (float) reward
				, state
			});
		}

		// Get the action. Scale to binary values and clamp
		auto action = this->selectAction(state);
		int8_t torque = (int8_t)(action * 127.0f);
		torque = max(min(torque, agentReads.maximumTorque), -agentReads.maximumTorque);

		// Send torque to main loop
		{
			registry.agentWrite({
				torque
				, this->frameTimer.getFrequency()
			});
		}
		// Remember trajectory variables
		{
			std::swap(this->priorState, state);
			this->priorAction = action;
			this->hasPriorState = true;
		}
	}

	//----------
	float
	Agent::selectAction(const State & state)
	{
		if(!this->initialised) {
			printf("[Agent] : Cannot selectAction. Not initialised\n");
			return 0.0f;
		}

		// Set input
		{
			memcpy(this->interpreter->input(0)->data.f, &state, sizeof(State));
		}

		// Invoke network
		{
			auto status = this->interpreter->Invoke();
			if(status != kTfLiteOk) {
				printf("[Agent] : Failed to invoke network\n");
				return 0.0f;
			}
		}

		// Get output
		{
			auto & result = this->interpreter->output(0)->data.f[0];
			printf("[Agent] : Invoke gives %f\n", result);
			return result;
		}
	}

	//----------
	void
	Agent::recordTrajectory(Trajectory && trajectory)
	{
		if(this->historyWritePosition <  localHistorySize) {
			this->history[this->historyWritePosition++] = trajectory;
		}
	}

	//----------
	bool
	Agent::checkInputSize()
	{
		const auto input = this->interpreter->input(0);
		auto dims = input->dims->size;
		if(dims != 2) {
			printf("[Agent] : Input Dimensions (%d) not equal to 2\n", dims);
			return false;
		}
		auto inputSize = input->dims->data[0] * input->dims->data[1];
		auto stateSize = sizeof(State) / sizeof(float);
		if(inputSize != stateSize) {
			printf("[Agent] : Size mismatch between State (%d) and network input (%d) \n", stateSize, inputSize);
			return false;
		}
		return true;
	}

	//----------
	bool
	Agent::checkOutputSize()
	{
		const auto output = this->interpreter->output(0);
		auto dims = output->dims->size;
		if(dims != 2) {
			printf("[Agent] : Output Dimensions (%d) not equal to 2\n", dims);
			return false;
		}
		auto outputSize = output->dims->data[0] * output->dims->data[1];
		if(outputSize != 1) {
			printf("[Agent] : Output size (%d) is not equal to 1 \n", outputSize);
			return false;
		}
		return true;
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

					// Allocate the tensors
					if(this->interpreter->AllocateTensors() != kTfLiteOk) {
						delete this->interpreter;
						printf("[Agent] : Failed to allocate tensors\n");
						this->initialised = false;
					}
				}
			}
		}

		// Check input and output size
		{
			printf("[Agent] : Succesfully initialised\n");
			this->initialised = this->checkInputSize() && this->checkOutputSize();
		}
	}
}