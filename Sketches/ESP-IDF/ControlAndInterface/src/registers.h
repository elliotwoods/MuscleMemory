std::map<RegisterType, Register> registers {
	{ RegisterType::DeviceID, {
		"DeviceID"
		, 1
		, Access::ReadWrite
		, 1
		, 1023
	}},


	{ RegisterType::MultiTurnPosition, {
		"MultiTurnPos"
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
		, 0 // kB
		, Access::ReadOnly
	}},
	{ RegisterType::CPUTemperature, {
		"CPUTemperature"
		, 0 // C
		, Access::ReadOnly
	}},
	{ RegisterType::UpTime, {
		"UpTime"
		, 0 // ms
		, Access::ReadOnly
	}},

	{ RegisterType::MotorControlFrequency, {
		"MCF"
		, 0 // Hz
		, Access::ReadOnly
	}},
	{ RegisterType::AgentControlFrequency, {
		"ACF"
		, 0 // Hz
		, Access::ReadOnly
	}},
	{ RegisterType::RegistryControlFrequency, {
		"RCF"
		, 0 // Hz
		, Access::ReadOnly
	}},

	{ RegisterType::AgentLocalHistorySize, {
		"LocalHistorySize"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::AgentTraining, {
		"Training"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::AgentNoiseAmplitude, {
		"NoiseAmplitude"
		, 0 // / 1000
		, Access::ReadOnly
	}},
	{ RegisterType::AgentAddProportional, {
		"AddProportional"
		, 0 // / 1000
		, Access::ReadOnly
	}},
	{ RegisterType::AgentAddConstant, {
		"AddConstant"
		, 0 // / 1000
		, Access::ReadOnly
	}},
};