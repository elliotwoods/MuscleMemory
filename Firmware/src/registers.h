{
	{ RegisterType::DeviceID, {
		"DeviceID"
		, 1
		, Access::ReadWrite
		, 1
		, 1023
	}},
	{ RegisterType::ControlMode, {
		"ControlMode"
		, 0 // 0=Standby, 1=PID, 2=Agent
		, Access::ReadWrite
		, 0
		, 3
	}},


	{ RegisterType::MultiTurnPosition, {
		"MultiTurnPos"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::Velocity, {
		"Velocity"
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
		, Access::ReadWrite
		, -128
		, 128
	}},
	{ RegisterType::MaximumTorque, {
		"MaximumTorque"
		, 32
		, Access::ReadWrite
		, 0
		, 128
	}},
	{ RegisterType::SoftLimitMin, {
		"SoftLimitMin"
		, - 64 * (1 << 14)
		, Access::ReadWrite
	}},
	{ RegisterType::SoftLimitMax, {
		"SoftLimitMax"
		, 64 * (1 << 14)
		, Access::ReadWrite
	}},
	{ RegisterType::DriveOffset, {
		"DriveOffset"
		, 64
		, Access::ReadWrite
		, -255
		, 255
	}},
	{ RegisterType::TargetPositionFiltered, {
		"TargetPositionFiltered"
		, 0
		, Access::ReadOnly
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
	{ RegisterType::EncoderPositionFilterSize, {
		"EncoderPositionFilterSize"
		, 1
		, Access::ReadWrite
		, 1
		, 255
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
	{ RegisterType::ShuntVoltage, {
		"ShuntVoltage"
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
	{ RegisterType::InterfaceEnabled, {
		"InterfaceEnabled"
		, 1
		, Access::ReadWrite
		, 0
		, 1
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
	{ RegisterType::MainLoopDelay, {
		"MainLoopDelay"
		, 10 // ms
		, Access::ReadWrite
		, 0
		, 1000
	}},

/*
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
*/

	{ RegisterType::PIDProportional, {
		"PIDProportional"
		, 100000
		, Access::ReadWrite
	}},
	{ RegisterType::PIDIntegral, {
		"PIDIntegral"
		, 0
		, Access::ReadWrite
	}},
	{ RegisterType::PIDDifferential, {
		"PIDDifferential"
		, 128
		, Access::ReadWrite
	}},
	{ RegisterType::PIDIntegralMax, {
		"PIDIntegralMax"
		, 2097152
		, Access::ReadWrite
	}},
	{ RegisterType::PIDResultP, {
		"PIDResultP"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::PIDResultI, {
		"PIDResultI"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::PIDResultD, {
		"PIDResultD"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::OffsetFactor, {
		"OffsetFactor"
		, 32
		, Access::ReadWrite
	}},
	{ RegisterType::OffsetMaximum, {
		"OffsetMaximum"
		, 64
		, Access::ReadWrite
		, 0
		, 255
	}},
	
	{ RegisterType::AntiStallEnabled, {
		"AntiStallEnabled"
		, 1
		, Access::ReadWrite
		, 0
		, 1
	}},
	{ RegisterType::AntiStallDeadZone, {
		"AntiStallDeadZone"
		, 200
		, Access::ReadWrite
		, 0
		, 1 << 16
	}},
	{ RegisterType::AntiStallMinVelocity, {
		"AntiStallMinVelocity"
		, 20000
		, Access::ReadWrite
		, 0
		, 1 << 20
	}},
	{ RegisterType::AntiStallAttack, {
		"AntiStallAttack"
		, 4
		, Access::ReadWrite
		, 0
		, 128
	}},
	{ RegisterType::AntiStallDecay, {
		"AntiStallDecay"
		, 8
		, Access::ReadWrite
		, 0
		, 128
	}},
	{ RegisterType::AntiStallValue, {
		"AntiStallValue"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::AntiStallScale, {
		"AntiStallScale"
		, 7
		, Access::ReadWrite
		, 0
		, 32
	}},

	{ RegisterType::CANRxThisFrame, {
		"CANRxThisFrame"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::CANTxThisFrame, {
		"CANTxThisFrame"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::CANErrorsThisFrame, {
		"CANErrorsThisFrame"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::CANErrorsTotal, {
		"CANErrorsTotal"
		, 0
		, Access::ReadOnly
	}},

	{ RegisterType::Reboot, {
		"Reboot"
		, 0
		, Access::ReadWrite
		, 0
		, 1
	}},
	{ RegisterType::ProvisioningEnabled, {
		"ProvisioningEnabled"
		, 1
		, Access::ReadWrite
		, 0
		, 1
	}},
	{ RegisterType::FastBoot, {
		"FastBoot"
		, 1
		, Access::ReadWrite
		, 0
		, 1
	}},
	{ RegisterType::CANWatchdogEnabled, {
		"CANWatchdogEnabled"
		, 1
		, Access::ReadWrite
		, 0
		, 1
	}},
	{ RegisterType::CANWatchdogTimeout, {
		"CANWatchdogTimeout"
		, 10 * 60 * 1000
		, Access::ReadWrite
	}},
	{ RegisterType::CANWatchdogTimer, {
		"CANWatchdogTimer"
		, 0
		, Access::ReadOnly
	}},

	{ RegisterType::OTADownloading, {
		"OTADownloading"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::OTAWritePosition, {
		"OTAWritePosition"
		, 0
		, Access::ReadOnly
	}},
	{ RegisterType::OTASize, {
		"OTASize"
		, 0
		, Access::ReadOnly
	}},
}