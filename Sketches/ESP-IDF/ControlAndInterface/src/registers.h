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

	{ RegisterType::MultiTurnPosition, {
		"MultiTurnPosition"
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
	{ RegisterType::Current, {
		"Current"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::MaximumCurrent, {
		"MaximumCurrent"
		, 2000 // value in mA
		, Access::ReadWrite
		, 0
		, 4000
	}},
	{ RegisterType::BusVoltage, {
		"BusVoltage"
		, 0
		, Access::ReadOnly
	}}
};