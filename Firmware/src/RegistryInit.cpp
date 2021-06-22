#include "Registry.h"

void
Registry::init()
{
	this->registers.emplace(
		RegisterType::DeviceID
		, Register {
			"DeviceID"
			, 1
			, Access::ReadWrite
			, 1
			, 1023
		}
	);

	this->registers.emplace(
		RegisterType::ControlMode
		, Register {
			"ControlMode"
			, 0 // 0=Standby, 1=PID, 2=Agent
			, Access::ReadWrite
			, 0
			, 3
		}
	);

	this->registers.emplace(
		RegisterType::ControlMode
		, Register {
			"ControlMode"
			, 0 // 0=Standby, 1=PID, 2=Agent
			, Access::ReadWrite
			, 0
			, 3
		}
	);


	this->registers.emplace(
		RegisterType::MultiTurnPosition
		, Register {
			"MultiTurnPos"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::Velocity
		, Register {
			"Velocity"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::TargetPosition
		, Register {
			"TargetPosition"
			, 0
			, Access::ReadWrite
		}
	);
	this->registers.emplace(
		RegisterType::Torque
		, Register {
			"Torque"
			, 0
			, Access::ReadWrite
			, -128
			, 128
		}
	);
	this->registers.emplace(
		RegisterType::MaximumTorque
		, Register {
			"MaximumTorque"
			, 32
			, Access::ReadWrite
			, 0
			, 128
		}
	);
	this->registers.emplace(
		RegisterType::SoftLimitMin
		, Register {
			"SoftLimitMin"
			, 0 // - 64 * (1 << 14)
			, Access::ReadWrite
		}
	);
	this->registers.emplace(
		RegisterType::SoftLimitMax
		, Register {
			"SoftLimitMax"
			, 0 //64 * (1 << 14)
			, Access::ReadWrite
		}
	);
	this->registers.emplace(
		RegisterType::DriveOffset
		, Register {
			"DriveOffset"
			, 64
			, Access::ReadWrite
			, -255
			, 255
		}
	);
	this->registers.emplace(
		RegisterType::TargetPositionFiltered
		, Register {
			"TargetPositionFiltered"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::MaxVelocity
		, Register {
			"MaxVelocity"
			, 0
			, Access::ReadWrite
		}
	);

	this->registers.emplace(
		RegisterType::EncoderReading
		, Register {
			"EncoderReading"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::EncoderErrors
		, Register {
			"EncoderErrors"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::EncoderPositionFilterSize
		, Register {
			"EncoderPositionFilterSize"
			, 1
			, Access::ReadWrite
			, 1
			, 255
		}
	);
	this->registers.emplace(
		RegisterType::MultiTurnSaveEnabled
		, Register {
			"MultiTurnSaveEnabled"
			, 1
			, Access::ReadWrite
			, 0
			, 1
		}
	);
	this->registers.emplace(
		RegisterType::MultiTurnSaveInterval
		, Register {
			"MultiTurnSaveInterval"
			, 1000
			, Access::ReadWrite
			, 0
			, std::numeric_limits<int32_t>::max()
		}
	);
	this->registers.emplace(
		RegisterType::MultiTurnLastSaveTime
		, Register {
			"MultiTurnLastSaveTime"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::ZeroPosSet
		, Register {
			"ZeroPosSet"
			, 0
			, Access::ReadWrite
			, 0
			, 1
		}
	);
	this->registers.emplace(
		RegisterType::ZeroPos
		, Register {
			"ZeroPos"
			, 0
			, Access::ReadWrite
		}
	);


	this->registers.emplace(
		RegisterType::Current
		, Register {
			"Current"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::BusVoltage
		, Register {
			"BusVoltage"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::ShuntVoltage
		, Register {
			"ShuntVoltage"
			, 0
			, Access::ReadOnly
		}
	);

	this->registers.emplace(
		RegisterType::FreeMemory
		, Register {
			"FreeMemory"
			, 0 // kB
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::Temperature
		, Register {
			"Temperature"
			, 0 // C
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::UpTime
		, Register {
			"UpTime"
			, 0 // ms
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::InterfaceEnabled
		, Register {
			"InterfaceEnabled"
			, 1
			, Access::ReadWrite
			, 0
			, 1
		}
	);
	

	this->registers.emplace(
		RegisterType::MotorControlFrequency
		, Register {
			"MCF"
			, 0 // Hz
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::AgentControlFrequency
		, Register {
			"ACF"
			, 0 // Hz
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::RegistryControlFrequency
		, Register {
			"RCF"
			, 0 // Hz
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::MainLoopDelay
		, Register {
			"MainLoopDelay"
			, 10 // ms
			, Access::ReadWrite
			, 0
			, 1000
		}
	);

	this->registers.emplace(
		RegisterType::AgentLocalHistorySize
		, Register {
			"LocalHistorySize"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::AgentTraining
		, Register {
			"Training"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::AgentNoiseAmplitude
		, Register {
			"NoiseAmplitude"
			, 0 // / 1000
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::AgentAddProportional
		, Register {
			"AddProportional"
			, 0 // / 1000
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::AgentAddConstant
		, Register {
			"AddConstant"
			, 0 // / 1000
			, Access::ReadOnly
		}
	);

	this->registers.emplace(
		RegisterType::PIDProportional
		, Register {
			"PIDProportional"
			, 100000
			, Access::ReadWrite
		}
	);
	this->registers.emplace(
		RegisterType::PIDIntegral
		, Register {
			"PIDIntegral"
			, 0
			, Access::ReadWrite
		}
	);
	this->registers.emplace(
		RegisterType::PIDDifferential
		, Register {
			"PIDDifferential"
			, 128
			, Access::ReadWrite
		}
	);
	this->registers.emplace(
		RegisterType::PIDIntegralMax
		, Register {
			"PIDIntegralMax"
			, 2097152
			, Access::ReadWrite
		}
	);
	this->registers.emplace(
		RegisterType::PIDResultP
		, Register {
			"PIDResultP"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::PIDResultI
		, Register {
			"PIDResultI"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::PIDResultD
		, Register {
			"PIDResultD"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::OffsetFactor
		, Register {
			"OffsetFactor"
			, 64
			, Access::ReadWrite
		}
	);
	this->registers.emplace(
		RegisterType::OffsetMinimum
		, Register {
			"OffsetMinimum"
			, 16
			, Access::ReadWrite
			, 0
			, 255
		}
	);
	this->registers.emplace(
		RegisterType::OffsetMaximum
		, Register {
			"OffsetMaximum"
			, 96
			, Access::ReadWrite
			, 0
			, 255
		}
	);
	
	this->registers.emplace(
		RegisterType::AntiStallEnabled
		, Register {
			"AntiStallEnabled"
			, 1
			, Access::ReadWrite
			, 0
			, 1
		}
	);
	this->registers.emplace(
		RegisterType::AntiStallDeadZone
		, Register {
			"AntiStallDeadZone"
			, 200
			, Access::ReadWrite
			, 0
			, 20
		}
	);
	this->registers.emplace(
		RegisterType::AntiStallMinVelocity
		, Register {
			"AntiStallMinVelocity"
			, 20000
			, Access::ReadWrite
			, 0
			, 1 << 20
		}
	);
	this->registers.emplace(
		RegisterType::AntiStallAttack
		, Register {
			"AntiStallAttack"
			, 4
			, Access::ReadWrite
			, 0
			, 128
		}
	);
	this->registers.emplace(
		RegisterType::AntiStallDecay
		, Register {
			"AntiStallDecay"
			, 8
			, Access::ReadWrite
			, 0
			, 128
		}
	);
	this->registers.emplace(
		RegisterType::AntiStallValue
		, Register {
			"AntiStallValue"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::AntiStallScale
		, Register {
			"AntiStallScale"
			, 7
			, Access::ReadWrite
			, 0
			, 32
		}
	);

	this->registers.emplace(
		RegisterType::CANRxThisFrame
		, Register {
			"CANRxThisFrame"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::CANTxThisFrame
		, Register {
			"CANTxThisFrame"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::CANErrorsThisFrame
		, Register {
			"CANErrorsThisFrame"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::CANErrorsTotal
		, Register {
			"CANErrorsTotal"
			, 0
			, Access::ReadOnly
		}
	);

	this->registers.emplace(
		RegisterType::Reboot
		, Register {
			"Reboot"
			, 0
			, Access::ReadWrite
			, 0
			, 1
		}
	);
	this->registers.emplace(
		RegisterType::ProvisioningEnabled
		, Register {
			"ProvisioningEnabled"
			, 1
			, Access::ReadWrite
			, 0
			, 1
		}
	);
	this->registers.emplace(
		RegisterType::FastBoot
		, Register {
			"FastBoot"
			, 1
			, Access::ReadWrite
			, 0
			, 1
		}
	);
	this->registers.emplace(
		RegisterType::CANWatchdogEnabled
		, Register {
			"CANWatchdogEnabled"
			, 1
			, Access::ReadWrite
			, 0
			, 1
		}
	);
	this->registers.emplace(
		RegisterType::CANWatchdogTimeout
		, Register {
			"CANWatchdogTimeout"
			, 10 * 60 * 1000
			, Access::ReadWrite
		}
	);
	this->registers.emplace(
		RegisterType::CANWatchdogTimer
		, Register {
			"CANWatchdogTimer"
			, 0
			, Access::ReadOnly
		}
	);

	this->registers.emplace(
		RegisterType::OTADownloading
		, Register {
			"OTADownloading"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::OTAWritePosition
		, Register {
			"OTAWritePosition"
			, 0
			, Access::ReadOnly
		}
	);
	this->registers.emplace(
		RegisterType::OTASize
		, Register {
			"OTASize"
			, 0
			, Access::ReadOnly
		}
	);
}
