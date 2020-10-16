#pragma once

#include "FreeRTOS.h"

#include <map>
#include <string>
#include <limits>

class Registry {
public:
	enum RegisterType : uint16_t {
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

		FreeMemory = 40
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

	struct ControlLoopWrites {
		int32_t encoderReading;
		int32_t encoderErrors;
		int32_t multiTurnPosition;
		int32_t torque;
		int32_t velocity;
	};

	struct ControlLoopReads {
		int32_t targetPosition;
		int8_t maximumTorque;
	};

	#include "registers.h"

	static Registry & X();

private:
	Registry();
public:
	void update();
	void controlLoopWrite(ControlLoopWrites &&);
	void controlLoopRead(ControlLoopReads &);
private:
	ControlLoopWrites controlLoopWritesIncoming, controlLoopWritesBack;
	SemaphoreHandle_t controlLoopWritesMutex;
	bool controlLoopWritesNew = false;

	ControlLoopReads controlLoopReads;
	SemaphoreHandle_t controlLoopReadsMutex;
};