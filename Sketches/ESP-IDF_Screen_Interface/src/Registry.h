#pragma once

#include <map>
#include <string>

class Registry {
public:
	enum RegisterType : uint16_t {
		deviceID = 0,

		CurrentPosition = 10,
		CurrentVelocity = 11,
		TargetPosition = 12,
		TargetVelocity = 13,

		CurrentI = 20,
		MaximumI = 21,
		CurrentVBus = 22
	};

	enum Operation : uint8_t {
		WriteRequest = 0,
		ReadRequest = 1,
		ReadResponse = 2
	};

	enum Access {
		ReadOnly
		, ReadWrite
	};
	
	struct Register {
		std::string name;
		int32_t value;
		int32_t min;
		int32_t max;
		Access access;
	};

	std::map<RegisterType, Register> registers {
		{ RegisterType::deviceID, {
			"deviceID"
			, 0
			, 0, 1024
			, Access::ReadWrite
		}},		
		{ RegisterType::CurrentPosition, {
			"CurrentPosition"
			, 0
			, std::numeric_limits<int32_t>::min()
			, std::numeric_limits<int32_t>::max()
			, Access::ReadOnly
		}},
		{ RegisterType::CurrentVelocity, {
			"CurrentVelocity"
			, 0
			, std::numeric_limits<int32_t>::min()
			, std::numeric_limits<int32_t>::max()
			, Access::ReadOnly
		}},
		{ RegisterType::TargetPosition, {
			"TargetPosition"
			, 0
			, std::numeric_limits<int32_t>::min()
			, std::numeric_limits<int32_t>::max()
			, Access::ReadWrite
		}},
		{ RegisterType::TargetVelocity, {
			"TargetVelocity"
			, 0
			, std::numeric_limits<int32_t>::min()
			, std::numeric_limits<int32_t>::max()
			, Access::ReadWrite
		}},
		{ RegisterType::CurrentI, {
			"CurrentI"
			, 0
			, 0, 32
			, Access::ReadOnly
		}},
		{ RegisterType::MaximumI, {
			"MaximumI"
			, 0
		}},
		{ RegisterType::CurrentVBus, {
			"CurrentVBus"
			, 0
		}}
	};

    static Registry & X();
private:
    Registry() { }
};