#pragma once

#include "freertos_include.h"

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

		// Position, Velocity, Target
		MultiTurnPosition = 10,
		Velocity = 11,
		TargetPosition = 12,
		TargetVelocity= 13,
		TargetPositionFiltered = 14,

		// Torque
		Torque = 15,
		MaximumTorque = 16,

		// Position and velocity limits
		SoftLimitMin = 20,
		SoftLimitMax = 21,
		MaxVelocity = 22,
		MaxPositionDeviation = 23,

		// Encoder
		EncoderReading = 30,
		EncoderErrors = 31,
		EncoderPositionFilterSize = 32,

		// Multi-turn
		MultiTurnSaveEnabled = 40,
		MultiTurnSaveInterval = 41,
		MultiTurnLastSaveTime = 42,
		ZeroPosSet = 43,
		ZeroPos = 44,

		// Power
		Current = 40,
		BusVoltage = 51,
		ShuntVoltage = 52,

		// System
		FreeMemory = 60,
		Temperature = 61,
		UpTime = 62,
		InterfaceEnabled = 63,

		// Update frequencies
		MotorControlFrequency = 70,
		AgentControlFrequency = 71,
		RegistryControlFrequency = 72,
		MainLoopDelay = 73,

		// Offest control
		DriveOffset = 80,
		OffsetFactor = 81,
		OffsetMinimum = 82,
		OffsetMaximum = 83,

		// Agent
		AgentLocalHistorySize = 100,
		AgentTraining = 101,
		AgentNoiseAmplitude = 102,
		AgentAddProportional = 103,
		AgentAddConstant = 104,

		// PID controller
		PIDProportional = 200,
		PIDIntegral = 201,
		PIDDifferential = 202,
		PIDIntegralMax = 203,
		PIDResultP = 204,
		PIDResultI = 205,
		PIDResultD = 206,

		// Anti-stall
		AntiStallEnabled = 250,
		AntiStallDeadZone = 251,
		AntiStallMinVelocity = 252,
		AntiStallAttack = 253,
		AntiStallDecay = 254,
		AntiStallValue = 255,
		AntiStallScale = 256,

		// Commmunications
		CANRxThisFrame = 300,
		CANTxThisFrame = 301,
		CANErrorsThisFrame = 302,
		CANErrorsTotal = 303,

		// Boot
		Reboot = 400,
		ProvisioningEnabled = 401,
		FastBoot = 402,
		CANWatchdogEnabled = 403,
		CANWatchdogTimeout = 404,
		CANWatchdogTimer = 405,

		// OTA
		OTADownloading = 450,
		OTAWritePosition = 451,
		OTASize = 452,

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