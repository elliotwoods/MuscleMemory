#include "WebSockets.h"
#include "Registry.h"

#include <WebSocketsClient.h>
#include "Devices/WiFi.h"
#include "WifiConfig.h"

#define WEBSOCKETS_DEBUG

//WebSocketsClient library conflicts with tensorflow
// Since Wifi is a singleton, this is safe to keep it here
WebSocketsClient webSocketsClient;

Interface::WebSockets * webSocketsInstance = nullptr;

//----------
void
webSocketEvent(WStype_t eventType, uint8_t * payload, size_t length)
{
	switch(eventType) {
		case WStype_t::WStype_BIN:
			webSocketsInstance->processIncomingRequests(payload, length);
		break;
		default:
		break;
	}
}

namespace Interface {
	//----------
	WebSockets::WebSockets(const Control::EncoderCalibration & encoderCalibration)
	: encoderCalibration(encoderCalibration)
	{

	}

	//----------
	void
	WebSockets::init()
	{
		webSocketsInstance = this;
		webSocketsClient.begin(MUSCLE_MEMORY_SERVER_HOST
				, MUSCLE_MEMORY_SERVER_PORT
				, ("/client/" + Devices::Wifi::X().getMacAddress()).c_str());
			
		webSocketsClient.onEvent(webSocketEvent);

		// If we fail to connect to websockets the first time, then don't try later
		if(webSocketsClient.isConnected()) {
			webSocketsClient.setReconnectInterval(5000);
			this->initialised = true;
			this->update();
			printf("[WebSockets] Connected\n");
		}
		else {
			printf("[WebSockets] Connection failed\n");
		}
	}

	//----------
	void
	WebSockets::update()
	{
		if(!this->initialised) {
			// e.g. the provisioning scren might call update on us, but websockets might not be active
			// Also if we failed on startup, we might choose not to perform any future updates
			return;
		}

		webSocketsClient.loop();

		if(webSocketsClient.isConnected()) {
			if(this->needsSendRegisterInfo) {
				this->sendRegisterInfo();
			}
			if(this->needsSendEncoderCalibration) {
				this->sendEncoderCalibration();
			}
			this->sendRegisters();
		}
	}

	//----------
	void
	WebSockets::processIncomingRequests(uint8_t * data, size_t size)
	{
		msgpack_unpacked request;
		msgpack_unpacked_init(&request);
		
		size_t offset = 0;
		
		while(msgpack_unpack_next(&request, (const char *) data, size, &offset)) {
#ifdef WEBSOCKETS_DEBUG
			// Print the object
			printf("[WebSockets] Incoming request :");
			msgpack_object_print(stdout, request.data);
			printf("\n");
#endif

			// Requests should be map of actions
			if(request.data.type == msgpack_object_type::MSGPACK_OBJECT_MAP) {
				// Iterate through the actions
				for(size_t actionIndex=0; actionIndex<request.data.via.map.size; actionIndex++) {
					// Get the name into a standard C string
					auto name = new char[request.data.via.map.ptr[actionIndex].key.via.str.size + 1];
					memcpy(name, request.data.via.map.ptr[actionIndex].key.via.str.ptr, request.data.via.map.ptr[actionIndex].key.via.str.size);
					name[request.data.via.map.ptr[actionIndex].key.via.str.size] = '\0';
					
					this->processIncomingAction(name, request.data.via.map.ptr[actionIndex].val);

					delete[] name;
				}
			}
			// get key
			msgpack_unpack_next(&request, (const char *) data, size, &offset);
		}

		msgpack_unpacked_destroy(&request);
	}

	//----------
	void
	WebSockets::processIncomingAction(const char * name, const msgpack_object & value)
	{
		static auto & registry = Registry::X();

#ifdef WEBSOCKETS_DEBUG
		msgpack_object_print(stdout, value);
		printf("[WebSockets] Performing action %s\n", name);
#endif

		if(strcmp(name, "register_values") == 0) {
#ifdef WEBSOCKETS_DEBUG
			printf("[WebSockets] Is register_values\n");
#endif
			// should be a map
			if(value.type == msgpack_object_type::MSGPACK_OBJECT_MAP) {
#ifdef WEBSOCKETS_DEBUG
				printf("Is map\n");
#endif
				for(size_t i=0; i<value.via.map.size; i++) {
					const auto & it = value.via.map.ptr[i];

					// get key as int or string
					if(it.key.type == msgpack_object_type::MSGPACK_OBJECT_POSITIVE_INTEGER) {
#ifdef WEBSOCKETS_DEBUG
						printf("[WebSockets] Key is positive integer\n");
#endif
						this->processIncomingRegisterValue(
							(Registry::RegisterType) it.key.via.i64
							, it.val
						);
					}
					else if(it.key.type == msgpack_object_type::MSGPACK_OBJECT_STR) {
#ifdef WEBSOCKETS_DEBUG
						printf("Key is string\n");
#endif
						this->processIncomingRegisterValue(
							(Registry::RegisterType) atoi(it.key.via.str.ptr)
							, it.val
						);
					}
				}
			}
		}
		else if (strcmp(name, "request_register_info") == 0) {
			this->needsSendRegisterInfo = true;
		}
		else if (strcmp(name, "request_encoder_calibration_data") == 0) {
			this->needsSendEncoderCalibration = true;
		}
		else if (strcmp(name, "register_save_default") == 0) {
			// should be an array
			if(value.type == msgpack_object_type::MSGPACK_OBJECT_ARRAY) {
#ifdef WEBSOCKETS_DEBUG
				printf("[WebSockets] SaveDefault : Is Array\n");
#endif
				for(size_t i=0; i<value.via.array.size; i++) {
					const auto & it = value.via.array.ptr[i];
					// register name
					if(it.type == msgpack_object_type::MSGPACK_OBJECT_STR) {
						// find the register by name
#ifdef WEBSOCKETS_DEBUG
						bool foundRegister = false;
#endif
						for(const auto & itRegister : registry.registers) {
							if(strcmp(it.via.str.ptr, itRegister.second.name.c_str()) == 0) {
								registry.saveDefault(itRegister.first);
#ifdef WEBSOCKETS_DEBUG
								foundRegister = true;
#endif
								break;
							}
						}
#ifdef WEBSOCKETS_DEBUG
						if(!foundRegister) {
							char buffer[it.via.str.size + 1];
							memcpy(buffer, it.via.str.ptr, it.via.str.size);
							buffer[it.via.str.size] = '\0';
							printf("[WebSockets] register_save_default : Failed to find register named '%s'z\n", buffer);
						}
#endif
					}
					else if(it.type == msgpack_object_type::MSGPACK_OBJECT_POSITIVE_INTEGER) {
						// by value
						registry.saveDefault((Registry::RegisterType) it.via.i64);
					}
				}
			}

			// This information has changed, so update the server (ideally we want to only update specific info)
			this->needsSendRegisterInfo = true;
		}
	}

	//----------
	void
	WebSockets::processIncomingRegisterValue(const Registry::RegisterType & registerType, const msgpack_object & value)
	{
		static auto & registry = Registry::X();
		auto findRegister = registry.registers.find(registerType);
		if(findRegister == registry.registers.end()) {
			printf("[WebSockets] : Error, register type (%d) not found\n", (int) registerType);
		}

		switch(value.type) {
			case msgpack_object_type::MSGPACK_OBJECT_BOOLEAN:
				findRegister->second.value = value.via.boolean;
			break;

			case msgpack_object_type::MSGPACK_OBJECT_POSITIVE_INTEGER:
			case msgpack_object_type::MSGPACK_OBJECT_NEGATIVE_INTEGER:
				findRegister->second.value = value.via.i64;
			break;

			case msgpack_object_type::MSGPACK_OBJECT_FLOAT32:
			case msgpack_object_type::MSGPACK_OBJECT_FLOAT64:
				findRegister->second.value = value.via.f64;
			break;

			default:
			break;
		}

		if(registerType == Registry::RegisterType::TargetPosition) {
			Control::FilteredTarget::X().notifyTargetChange();
		}
	}

	//-----------
	void
	WebSockets::sendRegisterInfo()
	{
		printf("[WebSockets] : sendRegisterInfo()\n");

		static auto & registry = Registry::X();

		// Initialise the buffer and packer
		msgpack_sbuffer buffer;
		msgpack_sbuffer_init(&buffer);
		msgpack_packer packer;
		msgpack_packer_init(&packer, &buffer, msgpack_sbuffer_write);

		msgpack_pack_map(&packer, 1);
		{
			// Key
			msgpack_pack_str_with_body(&packer, "register_info", 13);

			// Value
			msgpack_pack_map(&packer, registry.registers.size());
			for(const auto & it : registry.registers) {
				// Key
				msgpack_pack_uint16(&packer, (uint16_t) it.first);

				// Value
				msgpack_pack_map(&packer, 4);
				{
					// Key
					msgpack_pack_str_with_body(&packer, "name", 4);
					// Value
					msgpack_pack_str_with_body(&packer, it.second.name.c_str(), it.second.name.length());

					// Key
					msgpack_pack_str_with_body(&packer, "range", 5);
					if(it.second.range.limited) {
						msgpack_pack_map(&packer, 2);

						msgpack_pack_str_with_body(&packer, "min", 3);
						msgpack_pack_int32(&packer, it.second.range.min);

						msgpack_pack_str_with_body(&packer, "max", 3);
						msgpack_pack_int32(&packer, it.second.range.max);
					}
					else {
						msgpack_pack_nil(&packer);
					}

					// Key
					msgpack_pack_str_with_body(&packer, "access", 6);
					// Value
					msgpack_pack_uint8(&packer, (uint8_t) it.second.access);

					// Key
					msgpack_pack_str_with_body(&packer, "defaultValue", 12);
					// Value
					msgpack_pack_int32(&packer, it.second.defaultValue);
				}
			}
		}

		// Send and destroy the buffer
		webSocketsClient.sendBIN((const uint8_t*) buffer.data, buffer.size);
		msgpack_sbuffer_destroy(&buffer);

		this->needsSendRegisterInfo = false;
	}

	//-----------
	void
	WebSockets::sendRegisters()
	{
		static auto & registry = Registry::X();

		// Initialise the buffer and packer
		msgpack_sbuffer buffer;
		msgpack_sbuffer_init(&buffer);
		msgpack_packer packer;
		msgpack_packer_init(&packer, &buffer, msgpack_sbuffer_write);

		// Map
		msgpack_pack_map(&packer, 1);
		{
			// Key
			msgpack_pack_str_with_body(&packer, "register_values", 15);

			// Value - Map
			msgpack_pack_map(&packer, registry.registers.size());
			for(const auto & it : registry.registers) {
				// Key
				msgpack_pack_uint16(&packer, (uint16_t) it.first);

				// Value
				msgpack_pack_int32(&packer, it.second.value);
			}
		}

		// Send and destroy the buffer
		webSocketsClient.sendBIN((const uint8_t*) buffer.data, buffer.size);
		msgpack_sbuffer_destroy(&buffer);
	}

	//-----------
	void
	WebSockets::sendEncoderCalibration()
	{
		printf("[WebSockets] : sendEncoderCalibration()\n");

		// Initialise the buffer and packer
		msgpack_sbuffer buffer;
		msgpack_sbuffer_init(&buffer);
		msgpack_packer packer;
		msgpack_packer_init(&packer, &buffer, msgpack_sbuffer_write);

		// Map
		msgpack_pack_map(&packer, 1);
		{
			// Key
			msgpack_pack_str_with_body(&packer, "encoder_calibration", 19);

			// Value
			if(!this->encoderCalibration.getHasCalibration()) {
				msgpack_pack_nil(&packer);
			}
			else {
				const auto & stepCycleCalibration = this->encoderCalibration.getStepCycleCalibration();

				// Map
				msgpack_pack_map(&packer, 4);
				{
					// Key, Value
					msgpack_pack_str_with_body(&packer, "stepCycleOffset", 15);
					msgpack_pack_uint8(&packer, stepCycleCalibration.stepCycleOffset);

					// Key, Value
					msgpack_pack_str_with_body(&packer, "stepCycleCount", 14);
					msgpack_pack_uint16(&packer, stepCycleCalibration.stepCycleCount);

					// Key, Value
					msgpack_pack_str_with_body(&packer, "encoderPerStepCycle", 19);
					msgpack_pack_uint16(&packer, stepCycleCalibration.encoderPerStepCycle);
					
					// Key, Value
					msgpack_pack_str_with_body(&packer, "encoderValuePerStepCycle", 24);
					msgpack_pack_array(&packer, stepCycleCalibration.stepCycleCount);
					for(uint16_t i=0; i<stepCycleCalibration.stepCycleCount; i++) {
						msgpack_pack_uint16(&packer, stepCycleCalibration.encoderValuePerStepCycle[i]);
					}
				}
			}
		}

		// Send and destroy the buffer
		webSocketsClient.sendBIN((const uint8_t*) buffer.data, buffer.size);
		msgpack_sbuffer_destroy(&buffer);

		this->needsSendEncoderCalibration = false;
	}
}