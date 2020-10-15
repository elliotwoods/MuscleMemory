std::map<RegisterType, Register> registers {
	{ RegisterType::DeviceID, {
		"DeviceID"
		, 1
		, Access::ReadWrite
		, 1
		, 1023
	}},

	{ RegisterType::EncoderReading, {
		"EncoderReading"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::EncoderErrors, {
		"EncoderErrors"
		, 0
		, Access::ReadOnly
	}},

	{ RegisterType::Position, {
		"Position"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::Velocity, {
		"Veloctity"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::TargetPosition, {
		"TargetPosition"
		, 0
		, Access::ReadWrite
	}},
	{ RegisterType::TargetVelocity, {
		"TargetVelocity"
		, 0
		, Access::ReadWrite
	}},
	{ RegisterType::Current, {
		"Current"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::MaximumCurrent, {
		"MaximumCurrent"
		, 0
		, Access::ReadWrite
		, 0
		, 4
	}},
	{ RegisterType::BusVoltage, {
		"BusVoltage"
		, 0
		, Access::ReadOnly
	}}
};