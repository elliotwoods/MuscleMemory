#pragma once

#include <map>
#include <string>
#include <limits>

class Registry {
public:
	enum RegisterType : uint16_t {
		DeviceID = 0,

		Errors = 5,

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
	
	struct Register {
		const std::string name;
		int32_t value;
		const Range range;
		const Access access;
	};

	std::map<RegisterType, Register> registers {
		{ RegisterType::DeviceID, {
			"DeviceID"
			, 1
			, Range {
				true
				, 1
				, 1023
			}
			, Access::ReadWrite
		}},
		{ RegisterType::Errors, {
			"Errors"
			, 0
			, Range {
				false
			}
			, Access::ReadWrite
		}},
		{ RegisterType::Position, {
			"Position"
			, 0
			, Range {
				false
			}
			, Access::ReadOnly
		}},
		{ RegisterType::Velocity, {
			"Veloctity"
			, 0
			, Range {
				false
			}
			, Access::ReadOnly
		}},
		{ RegisterType::TargetPosition, {
			"TargetPosition"
			, 0
			, Range {
				false
			}
			, Access::ReadWrite
		}},
		{ RegisterType::TargetVelocity, {
			"TargetVelocity"
			, 0
			, Range {
				false
			}
			, Access::ReadWrite
		}},
		{ RegisterType::Current, {
			"Current"
			, 0
			, Range {
				false
			}
			, Access::ReadOnly
		}},
		{ RegisterType::MaximumCurrent, {
			"MaximumCurrent"
			, 0
			, Range {
				true
				, 0
				, 4
			}
			, Access::ReadWrite
		}},
		{ RegisterType::BusVoltage, {
			"BusVoltage"
			, 0
			, Range {
				true
				, 0
				, 4
			}
			, Access::ReadOnly
		}}
	};

	static Registry & X();
private:
	Registry() { }
};