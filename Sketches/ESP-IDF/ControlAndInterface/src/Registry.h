#pragma once

#include "FreeRTOS.h"

#include <map>
#include <string>
#include <limits>

class Registry {
public:
	enum RegisterType : uint16_t {
		DeviceID = 0,

		EncoderReading = 5,
		EncoderErrors = 6,

		Position = 10,
		Velocity = 11,
		TargetPosition = 12,
		TargetVelocity = 13,

		Current = 20,
		MaximumCurrent = 21,
		BusVoltage = 22
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
		
		const std::string & getName();
		const Access & getAccess();
		const Range & getRange();

		int32_t getValue();
		void  setValue(int32_t);
	
		const std::string name;
		int32_t value;
		const Access access;
		const Range range;
	};

	struct ControlLoopWrites {
		int32_t encoderReading;
		int32_t encoderErrors;
		int32_t position;
	};

	#include "registers.h"

	static Registry & X();

private:
	Registry();
public:
	void update();
	void controlLoopWrite(ControlLoopWrites &&);
private:
	ControlLoopWrites controlLoopWritesIncoming, controlLoopWritesBack;
	SemaphoreHandle_t controlLoopWritesMutex;
	bool controlLoopWritesNew = false;
};