#include "WebSockets.h"
#include "Registry.h"

#include <WebSocketsClient.h>
#include "Devices/WiFi.h"
#include "WifiConfig.h"

#define WEBSOCKETS_DEBUG

//WebSocketsClient library conflicts with tensorflow
// Since Wifi is a singleton, this is safe to keep it here
WebSocketsClient webSocketsClient;

Control::WebSockets * webSocketsInstance = nullptr;

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

namespace Control {
	//----------
	void
	WebSockets::init()
	{
		webSocketsInstance = this;
		webSocketsClient.begin(MUSCLE_MEMORY_SERVER_HOST
				, MUSCLE_MEMORY_SERVER_PORT
				, ("/client/" + Devices::Wifi::X().getMacAddress()).c_str());
			
		webSocketsClient.onEvent(webSocketEvent);
		webSocketsClient.setReconnectInterval(5000);

		this->update();
	}

	//----------
	void
	WebSockets::update()
	{
		webSocketsClient.loop();

		if(webSocketsClient.isConnected()) {
			if(this->needsSendRegisterInfo) {
				this->sendRegisterInfo();
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
				msgpack_pack_map(&packer, 3);
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
}