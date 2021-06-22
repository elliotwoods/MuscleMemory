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
#include <atomic>

#include "DataTypes.h"
#include "Utils/FrameTimer.h"

//#define ATOMIC_REGISTERS

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
		TargetPositionFiltered = 17,
		MaxVelocity = 18,

		// Encoder
		EncoderReading = 20,
		EncoderErrors = 21,
		EncoderPositionFilterSize = 22,
		MultiTurnSaveEnabled = 25,
		MultiTurnSaveInterval = 26,
		MultiTurnLastSaveTime = 27,
		ZeroPosSet = 28,
		ZeroPos = 29,

		// Power
		Current = 30,
		BusVoltage = 32,
		ShuntVoltage = 33,

		// System
		FreeMemory = 40,
		Temperature = 41,
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
		PIDResultP = 74,
		PIDResultI = 75,
		PIDResultD = 76,

		// Offest control
		DriveOffset = 80,
		OffsetFactor = 81,
		OffsetMinimum = 82,
		OffsetMaximum = 83,

		// Anti-stall
		AntiStallEnabled = 90,
		AntiStallDeadZone = 91,
		AntiStallMinVelocity = 92,
		AntiStallAttack = 93,
		AntiStallDecay = 94,
		AntiStallValue = 95,
		AntiStallScale = 96,

		// CAN debug
		CANRxThisFrame = 150,
		CANTxThisFrame = 151,
		CANErrorsThisFrame = 152,
		CANErrorsTotal = 153,

		// Boot
		Reboot = 200,
		ProvisioningEnabled = 201,
		FastBoot = 202,
		CANWatchdogEnabled = 203,
		CANWatchdogTimeout = 204,
		CANWatchdogTimer = 205,

		// OTA
		OTADownloading = 210,
		OTAWritePosition = 211,
		OTASize = 212,

		PrimaryRegister = TargetPosition
	};

	enum Operation : uint8_t {
		ReadRequest = 0,
		WriteRequest = 1,
		ReadResponse = 2,
		WriteAndSaveDefaultRequest = 3,

		Ping = 8,
		PingResponse = 9,

		OTARequests = 100, // mark the start of OTA requests in the enum
		OTAInfo = 100,
		OTAData = 101,
		OTARequestInfo = 102,
		OTARequestData = 103,
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
		Register(const Register &);
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
#ifdef ATOMIC_REGISTERS
		std::atomic<int32_t> value;
#else
		int32_t value;
#endif
		int32_t defaultValue;
		const Access access;
		const Range range;
	};

	std::map<RegisterType, Register> registers;

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
	void init();
public:
	void update();

	void loadDefaults();
	void saveDefault(const RegisterType & );
private:
	MotorControlReads motorControlReads;
	SemaphoreHandle_t motorControlReadsMutex;
	Utils::FrameTimer frameTimer;

	std::set<RegisterType> defaultsToSave;
};

#ifdef ATOMIC_REGISTERS
int32_t getRegisterValue(const Registry::RegisterType &);
#else
const int32_t & getRegisterValue(const Registry::RegisterType &);
#endif
const Registry::Range & getRegisterRange(const Registry::RegisterType &);
void setRegisterValue(const Registry::RegisterType &, int32_t value);