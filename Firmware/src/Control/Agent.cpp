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
	#include "thirdparty/mbedtls/base64.h"
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
		this->historyWrites = new History();
		this->historyReads = new History();
	}

	//----------
	Agent::~Agent()
	{
		heap_caps_free(this->heapArea);
		this->heapArea = nullptr;

		delete this->historyWrites;
		delete this->historyReads;
	}

	//----------
	void
	Agent::init()
	{
		// Start the session on the server
		{
			// Make the request
			cJSON * response = nullptr;
			{
				auto request = cJSON_CreateObject();
				cJSON_AddStringToObject(request, "client_id", Devices::Wifi::X().getMacAddress().c_str());
				cJSON_AddBoolToObject(request, "recycle_session", true);
				response = Devices::Wifi::X().post("/startSession", request);
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

		// Setup threading objects for server communication
		this->historyToServer = xQueueCreate(1, sizeof(History*));
		this->historyMutex = xSemaphoreCreateMutex();
	}

	//----------
	void IRAM_ATTR
	Agent::update()
	{
		this->frameTimer.update();

		// Read the state from registry
		const auto & multiTurnPosition = getRegisterValue(Registry::RegisterType::MultiTurnPosition);
		const auto & targetPosition = getRegisterValue(Registry::RegisterType::TargetPosition);
		const auto & velocity = getRegisterValue(Registry::RegisterType::Velocity);
		const auto & motorControlFrequency = getRegisterValue(Registry::RegisterType::MotorControlFrequency);
		const auto & current = getRegisterValue(Registry::RegisterType::Current);
		const auto & maximumTorque = getRegisterValue(Registry::RegisterType::MaximumTorque);

		// Prepare the state
		Agent::State state;
		{
			state.position = float(multiTurnPosition) / float(1 << 14);
			state.targetMinusPosition = float(targetPosition - multiTurnPosition) / float(1 << 14);
			state.velocity = float(velocity) / float(1 << 14);
			state.agentFrequency = float(this->frameTimer.getFrequency()) / 1000.0f;
			state.motorControlFrequency = float(motorControlFrequency) / 1000.0f;
			state.current = float(current) / 1000.0f;
		}

		// Record trajectory if we have a prior state
		if(this->runtimeParameters.isTraining) {
			if (this->hasPriorState) {
				int32_t reward = -abs(state.targetMinusPosition);
				this->recordTrajectory({
					this->priorState
					, this->priorAction
					, (float) reward
					, state
				});
			}
		}

		// Get the action. Scale to binary values and clamp
		auto action = this->selectAction(state);
		int8_t torque = (int8_t)(action * 127.0f);
		torque = max(min(torque, maximumTorque), -maximumTorque);

		// Registry writes
		{
			setRegisterValue(Registry::RegisterType::Torque, torque);
			setRegisterValue(Registry::RegisterType::AgentControlFrequency, this->frameTimer.getFrequency());
			setRegisterValue(Registry::RegisterType::AgentLocalHistorySize, this->historyWrites->writePosition);
			setRegisterValue(Registry::RegisterType::AgentTraining, this->runtimeParameters.isTraining);
			setRegisterValue(Registry::RegisterType::AgentNoiseAmplitude, this->runtimeParameters.noiseAmplitude * 1000.0f);
			setRegisterValue(Registry::RegisterType::AgentAddProportional, this->runtimeParameters.addProportional * 1000.0f);
			setRegisterValue(Registry::RegisterType::AgentAddConstant, this->runtimeParameters.addConstant * 1000.0f);
		}

		// Remember trajectory variables
		if(this->runtimeParameters.isTraining) {
			std::swap(this->priorState, state);
			this->priorAction = action;
			this->hasPriorState = true;
		}
		else {
			this->hasPriorState = false;
		}
	}

	//----------
	void
	Agent::loadModel(const char * data, size_t size)
	{
		this->unloadModel();
		this->modelString.assign(data, data + size);

		// Load the model
		{
			this->model = ::tflite::GetModel(this->modelString.data());
			if(this->model->version() != TFLITE_SCHEMA_VERSION) {
				printf("[Agent] - Model provided is schema version %d not equal "
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

	//----------
	void
	Agent::unloadModel()
	{
		if(this->interpreter) {
			delete this->interpreter;
			this->interpreter = nullptr;
		}

		if(this->model) {
			this->model = nullptr;	
		}

		this->modelString.clear();

		this->initialised = false;
	}

	//----------
	void
	Agent::serverCommunicateTask()
	{
		History * history;
		while(true) {
			if(xQueueReceive(this->historyToServer, &history, portMAX_DELAY)) {
				// Receieve the data into base64 encoding
				auto rawMessage = (const uint8_t *) history->trajectories;
				auto rawMessageLength = sizeof(Trajectory) * history->writePosition;
				size_t base64BufferSize = (rawMessageLength / 3 + 1) * 4;
				auto base64Text = (uint8_t *) malloc(base64BufferSize);
				size_t base64MessageLength;
				if(xSemaphoreTake(this->historyMutex, portMAX_DELAY)) {
					{
						// Encode the text
						auto result = mbedtls_base64_encode(base64Text
							, base64BufferSize
							, &base64MessageLength
							, (const uint8_t *) history->trajectories
							, rawMessageLength);
						
						// Ensure we have a standard C-string (this needs testing since we're using uint8_t)
						base64Text[base64BufferSize] = '\0';

						xSemaphoreGive(this->historyMutex);
					}
					
					bool jsonSerialiseFail = false;
					auto request = cJSON_CreateObject();
					if(!cJSON_AddStringToObject(request, "client_id", Devices::Wifi::X().getMacAddress().c_str())) {
						jsonSerialiseFail |= true;
					}
					if(!cJSON_AddStringToObject(request, "trajectories", (char *) base64Text)) {
						jsonSerialiseFail |= true;
					}
					free(base64Text);

					if(jsonSerialiseFail) {
						printf("[Agent] : Failed to serialise json when submitting trajectories\n");
						cJSON_Delete(request);
					}
					else {
						auto response = Devices::Wifi::X().post("/transmitTrajectories", request);
						cJSON_Delete(request);
						this->processIncoming(response);
						cJSON_Delete(response);
					}
				}

			}
		}
		
	}

	//----------
	float IRAM_ATTR
	Agent::selectAction(const State & state)
	{
		if(!this->initialised) {
			printf("[Agent] : Cannot selectAction. Not initialised\n");

			// Sometimes this happens sporadically - needs investigating. For the time being let it happen a few times
			static int count = 0;
			if(count++ > 100) {
				abort();
			}
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
			auto & action = this->interpreter->output(0)->data.f[0];

			// Apply noise and if we're training
			if(this->runtimeParameters.isTraining) {
				return action 
					+ this->runtimeParameters.noiseAmplitude * this->actionNoise.get()
					+ this->runtimeParameters.addProportional * state.targetMinusPosition
					+ this->runtimeParameters.addConstant;
			}
			else {
				return action;
			}
		}
	}

	//----------
	void IRAM_ATTR
	Agent::recordTrajectory(Trajectory && trajectory)
	{
		this->historyWrites->trajectories[this->historyWrites->writePosition++] = trajectory;
		if(this->historyWrites->writePosition == localHistorySize) {
			// Lock the history
			if(xSemaphoreTake(this->historyMutex, 5000 / portTICK_RATE_MS)) {
				// swap the buffers
				std::swap(this->historyReads, this->historyWrites);

				// reset the buffer which has now been assigned for writing
				this->historyWrites->writePosition = 0;

				// send to the server
				if(!xQueueSend(this->historyToServer, &this->historyReads, 5000 / portTICK_RATE_MS)) {
					printf("[Agent] : Could not send history");
				}

				xSemaphoreGive(this->historyMutex);
			}
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
		if(false) {
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
				// "model"
				{
					auto modelJson = cJSON_GetObjectItemCaseSensitive(contentJson, "model");
					if(modelJson && cJSON_IsString(modelJson) && modelJson->valuestring) {
						// Decode from BASE64 into local copy
						size_t base64Length = strlen(modelJson->valuestring);
						size_t binaryStringBufferLength = (base64Length / 3 + 1) * 4;
						auto binaryString = (uint8_t*) malloc(binaryStringBufferLength);
						size_t outputLength;

						auto result = mbedtls_base64_decode(binaryString
							, binaryStringBufferLength
							, &outputLength
							, (const unsigned char *) modelJson->valuestring
							, base64Length);
						
						this->loadModel((const char *) binaryString, outputLength);
						free(binaryString);
					}
				}
				
				// "runtime_parameters"
				{
					auto runtimeParametersJson = cJSON_GetObjectItemCaseSensitive(contentJson, "runtime_parameters");
					if(runtimeParametersJson && cJSON_IsObject(runtimeParametersJson)) {
						this->runtimeParameters.deserialize(runtimeParametersJson);
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