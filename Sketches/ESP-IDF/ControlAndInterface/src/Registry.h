#pragma once

#include "FreeRTOS.h"

#include <map>
#include <string>
#include <limits>

#include "DataTypes.h"
#include "Utils/FrameTimer.h"

class Registry {
public:
	enum class RegisterType : uint16_t {
		DeviceID = 0,


		MultiTurnPosition = 10,
		Velocity = 11,
		TargetPosition = 12,
		Torque = 13,
		MaximumTorque = 14,

		EncoderReading = 21,
		EncoderErrors = 22,

		Current = 30,
		BusVoltage = 32,

		FreeMemory = 40,
		CPUTemperature = 41,
		UpTime = 42,

		MotorControlFrequency = 50,
		AgentControlFrequency = 51,
		RegistryControlFrequency = 52,

		AgentLocalHistorySize = 60,
		AgentTraining = 61,
		AgentNoiseAmplitude = 62
	};

	enum Operation : uint8_t {
		WriteRequest = 0,
		ReadRequest = 1,
		ReadResponse = 2
	};

	struct Range {
		bool limited;
		int32_t min;
		int32_t max;
	};

	enum Access {
		ReadOnly
		, ReadWrite
	};
	
	class Register {
	public:
		Register(const std::string & name
			, int32_t value
			, Access);
		Register(const std::string & name
			, int32_t value
			, Access
			, int32_t min
			, int32_t max);
		/*
		// Not using this for now
		const std::string & getName();
		const Access & getAccess();
		const Range & getRange();

		int32_t getValue();
		void  setValue(int32_t);
		*/
	
		const std::string name;
		int32_t value;
		const Access access;
		const Range range;
	};

	#include "registers.h"

	struct MotorControlWrites {
		int32_t encoderReading;
		int32_t encoderErrors;
		int32_t multiTurnPosition;
		int32_t velocity;
		int32_t motorControlFrequency;
	};

	struct MotorControlReads {
		Torque torque;
	};

	struct AgentReads {
		MultiTurnPosition multiTurnPosition;
		Velocity velocity;
		MultiTurnPosition targetPosition;
		int32_t motorControlFrequency;
		int32_t current;
		Torque maximumTorque;
	};

	struct AgentWrites {
		Torque torque;
		int16_t agentFrequency;
		int16_t localHistorySize;
		bool isTraining;
		int16_t noiseAmplitude;
	};

	static Registry & X();

private:
	Registry();
	~Registry();
public:
	void update();
	void motorControlWrite(MotorControlWrites &&);
	void motorControlRead(MotorControlReads &);

	void agentWrite(AgentWrites &&);
	void agentRead(AgentReads &);
private:
	MotorControlReads motorControlReads;
	SemaphoreHandle_t motorControlReadsMutex;

	AgentReads agentReads;
	SemaphoreHandle_t agentReadsMutex;

	MotorControlWrites motorControlWritesIncoming, motorControlWritesBack;
	SemaphoreHandle_t motorControlWritesMutex;
	bool motorControlWritesNew = false;

	AgentWrites agentWritesIncoming, agentWritesBack;
	SemaphoreHandle_t agentWritesMutex;
	bool agentWritesNew = false;

	Utils::FrameTimer frameTimer;
};