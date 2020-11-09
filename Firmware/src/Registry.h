#pragma once

#ifdef ARDUINO
#include "FreeRTOS.h"
#else
#include <freertos/FreeRTOS.h>
#endif

#include <map>
#include <set>
#include <string>
#include <limits>

#include "DataTypes.h"
#include "Utils/FrameTimer.h"

class Registry {
public:
	enum class RegisterType : uint16_t {
		// Device
		DeviceID = 0,
		ControlMode = 1,

		// Position, Velocity, Target, Torque
		MultiTurnPosition = 10,
		Velocity = 11,
		TargetPosition = 12,
		Torque = 13,
		MaximumTorque = 14,
		SoftLimitMin = 15,
		SoftLimitMax = 16,

		// Encoder
		EncoderReading = 21,
		EncoderErrors = 22,
		EncoderPositionFilterSize = 23,

		// Power
		Current = 30,
		BusVoltage = 32,
		ShuntVoltage = 33,

		// System
		FreeMemory = 40,
		CPUTemperature = 41,
		UpTime = 42,
		InterfaceEnabled = 43,

		// Update frequencies
		MotorControlFrequency = 50,
		AgentControlFrequency = 51,
		RegistryControlFrequency = 52,
		MainLoopDelay = 53,

		// Agent
		AgentLocalHistorySize = 60,
		AgentTraining = 61,
		AgentNoiseAmplitude = 62,
		AgentAddProportional = 63,
		AgentAddConstant = 64,

		// PID controller
		PIDProportional = 70,
		PIDIntegral = 71,
		PIDDifferential = 72,
		PIDIntegralMax = 73,
		AntiStallEnabled = 74,
		AntiStallDeadZone = 75,
		AntiStallMinVelocity = 76,
		AntiStallAttack = 77,
		AntiStallDecay = 78,
		AntiStallValue = 79,

		// CAN debug
		CANRxThisFrame = 80,
		CANTxThisFrame = 81,
		CANErrorsThisFrame = 81,

		// Boot
		Reboot = 90,
		ProvisioningEnabled = 91,
	};

	enum Operation : uint8_t {
		ReadRequest = 0,
		WriteRequest = 1,
		ReadResponse = 2
	};

	static const std::set<RegisterType> defaultRegisterReads;

	struct Range {
		bool limited;
		int32_t min;
		int32_t max;
	};

	enum Access : uint8_t {
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
		SingleTurnPosition encoderReading;
		uint8_t encoderErrors;
		MultiTurnPosition multiTurnPosition;
		Velocity velocity;
		Frequency motorControlFrequency;
	};

	struct MotorControlReads {
		Torque torque;
		uint8_t encoderPositionFilterSize;
	};

	struct AgentReads {
		MultiTurnPosition multiTurnPosition;
		Velocity velocity;
		MultiTurnPosition targetPosition;
		int32_t motorControlFrequency;
		int32_t current;
		Torque maximumTorque;
		MultiTurnPosition softLimitMin;
		MultiTurnPosition softLimitMax;
	};

	struct AgentWrites {
		Torque torque;
		int16_t agentFrequency;
		int16_t localHistorySize;
		bool isTraining;
		int16_t noiseAmplitude;
		int16_t addProportional;
		int16_t addConstant;
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

	void loadDefaults();
	void saveDefault(const RegisterType & );
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

	std::set<RegisterType> defaultsToSave;
};