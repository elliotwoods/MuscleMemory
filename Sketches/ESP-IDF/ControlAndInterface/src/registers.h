std::map<RegisterType, Register> registers {
	{ RegisterType::DeviceID, {
		"DeviceID"
		, 1
		, Access::ReadWrite
		, 1
		, 1023
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
	{ RegisterType::Torque, {
		"Torque"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::MaximumTorque, {
		"MaximumTorque"
		, 32
		, Access::ReadWrite
		, 0
		, 128
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


	{ RegisterType::Current, {
		"Current"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::BusVoltage, {
		"BusVoltage"
		, 0
		, Access::ReadOnly
	}},

	{ RegisterType::FreeMemory, {
		"FreeMemory"
		, 0 // Value in kB
		, Access::ReadOnly
	}}
};