#include "Registry.h"

//----------
Registry::Register::Register(const std::string & name, int32_t value, Access access)
: name(name)
, value(value)
, access(access)
, range({false})
{

}

//----------
Registry::Register::Register(const std::string & name, int32_t value, Access access, int32_t min, int32_t max)
: name(name)
, value(value)
, access(access)
, range({true, min, max})
{
	
}

//----------
Registry&
Registry::X() {
	static Registry registry;
	return registry;
}

//----------
Registry::Registry()
{
	this->controlLoopWritesMutex = xSemaphoreCreateMutex();
	this->controlLoopReadsMutex = xSemaphoreCreateMutex();
}

//----------
void
Registry::update()
{
	// Handle data outgoing to control loop
	if(xSemaphoreTake(this->controlLoopReadsMutex, portMAX_DELAY)) {
		this->controlLoopReads.targetPosition = this->registers.at(RegisterType::TargetPosition).value;
		this->controlLoopReads.maximumTorque = this->registers.at(RegisterType::MaximumTorque).value;
		xSemaphoreGive(this->controlLoopReadsMutex);
	}

	// Handle data incoming from control loop
	{
		bool newData = false;

		// Write all data incoming from control loop into registers
		if(xSemaphoreTake(this->controlLoopWritesMutex, portMAX_DELAY)) {
			if(this->controlLoopWritesNew) {
				std::swap(this->controlLoopWritesIncoming, this->controlLoopWritesBack);
				this->controlLoopWritesNew = false;
				newData = true;
				xSemaphoreGive(this->controlLoopWritesMutex);
			}
		}

		if(newData) {
			this->registers.at(RegisterType::EncoderReading).value = this->controlLoopWritesBack.encoderReading;
			this->registers.at(RegisterType::EncoderErrors).value = this->controlLoopWritesBack.encoderErrors;
			this->registers.at(RegisterType::MultiTurnPosition).value = this->controlLoopWritesBack.multiTurnPosition;
			this->registers.at(RegisterType::Torque).value = this->controlLoopWritesBack.torque;
			this->registers.at(RegisterType::Velocity).value = this->controlLoopWritesBack.velocity;
		}
	}
}

//----------
void
Registry::controlLoopWrite(ControlLoopWrites && controlLoopWrites)
{
	// Write incoming control loop writes into the incoming buffer (previous data is overwritten)
	if(xSemaphoreTake(this->controlLoopWritesMutex, portMAX_DELAY)) {
		std::swap(controlLoopWrites, this->controlLoopWritesIncoming);
		this->controlLoopWritesNew = true;
		xSemaphoreGive(this->controlLoopWritesMutex);
	}
}

//----------
void
Registry::controlLoopRead(ControlLoopReads & controlLoopReads)
{
	
	if(xSemaphoreTake(this->controlLoopReadsMutex, portMAX_DELAY)) {
		controlLoopReads = this->controlLoopReads;
		xSemaphoreGive(this->controlLoopReadsMutex);
	}
}