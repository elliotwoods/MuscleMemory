#include "Registry.h"

const auto waitTime = portMAX_DELAY;

const char registerDefaultsFilename[] = "/appdata/registerDefault.dat";

//----------
const std::set<Registry::RegisterType>
Registry::defaultRegisterReads {
		Registry::RegisterType::MultiTurnPosition
		, Registry::RegisterType::Torque
		, Registry::RegisterType::EncoderReading
		, Registry::RegisterType::EncoderErrors
		, Registry::RegisterType::Current
		, Registry::RegisterType::BusVoltage
		, Registry::RegisterType::FreeMemory
		, Registry::RegisterType::CPUTemperature
		, Registry::RegisterType::UpTime
		, Registry::RegisterType::MotorControlFrequency
		, Registry::RegisterType::AgentControlFrequency
		, Registry::RegisterType::RegistryControlFrequency
		, Registry::RegisterType::AgentLocalHistorySize
		, Registry::RegisterType::AgentTraining
	};

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
	this->motorControlWritesMutex = xSemaphoreCreateMutex();
	this->motorControlReadsMutex = xSemaphoreCreateMutex();
	this->agentReadsMutex = xSemaphoreCreateMutex();
	this->agentWritesMutex = xSemaphoreCreateMutex();

	this->frameTimer.init();
}

//----------
Registry::~Registry()
{
	vSemaphoreDelete(this->motorControlWritesMutex);
	vSemaphoreDelete(this->motorControlReadsMutex);
	vSemaphoreDelete(this->agentReadsMutex);
	vSemaphoreDelete(this->agentWritesMutex);
}

//----------
void
Registry::update()
{
	// Update our own frame timer
	{
		this->frameTimer.update();
		this->registers.at(RegisterType::RegistryControlFrequency).value = this->frameTimer.getFrequency();
	}

	// Handle data outgoing to motor control loop
	if(xSemaphoreTake(this->motorControlReadsMutex, waitTime)) {
		this->motorControlReads.torque = this->registers.at(RegisterType::Torque).value;
		this->motorControlReads.encoderPositionFilterSize = this->registers.at(RegisterType::EncoderPositionFilterSize).value;
		xSemaphoreGive(this->motorControlReadsMutex);
	}

	// Handle data outgoing to agent control loop
	{
		if(xSemaphoreTake(this->agentReadsMutex, waitTime)) {
			this->agentReads.multiTurnPosition = this->registers.at(RegisterType::MultiTurnPosition).value;
			this->agentReads.velocity = this->registers.at(RegisterType::Velocity).value;
			this->agentReads.targetPosition = this->registers.at(RegisterType::TargetPosition).value;
			this->agentReads.motorControlFrequency = this->registers.at(RegisterType::MotorControlFrequency).value;
			this->agentReads.current = this->registers.at(RegisterType::Current).value;
			this->agentReads.maximumTorque = this->registers.at(RegisterType::MaximumTorque).value;

			// Soft limits
			{
				const auto & min = this->registers.at(RegisterType::SoftLimitMin).value;
				const auto & max = this->registers.at(RegisterType::SoftLimitMax).value;
				this->agentReads.softLimitMin = min;
				this->agentReads.softLimitMax = max;

				if(this->agentReads.targetPosition > max) {
					this->agentReads.targetPosition = max;
				}
				else if(this->agentReads.targetPosition < min) {
					this->agentReads.targetPosition = min;
				}
			}
			
			xSemaphoreGive(this->agentReadsMutex);
		}
	}

	// Handle data incoming from motor control loop
	{
		bool newData = false;

		// Write all data incoming from control loop into registers
		if(xSemaphoreTake(this->motorControlWritesMutex, waitTime)) {
			if(this->motorControlWritesNew) {
				std::swap(this->motorControlWritesIncoming, this->motorControlWritesBack);
				this->motorControlWritesNew = false;
				newData = true;
			}
			xSemaphoreGive(this->motorControlWritesMutex);
		}

		if(newData) {
			this->registers.at(RegisterType::EncoderReading).value = this->motorControlWritesBack.encoderReading;
			this->registers.at(RegisterType::EncoderErrors).value = this->motorControlWritesBack.encoderErrors;
			this->registers.at(RegisterType::MultiTurnPosition).value = this->motorControlWritesBack.multiTurnPosition;
			this->registers.at(RegisterType::Velocity).value = this->motorControlWritesBack.velocity;
			this->registers.at(RegisterType::MotorControlFrequency).value = this->motorControlWritesBack.motorControlFrequency;
		}
	}

	// Handle data incoming from agent control loop
	{
		bool newData = false;

		// Write all data incoming from control loop into registers
		if(xSemaphoreTake(this->agentWritesMutex, waitTime)) {
			if(this->agentWritesNew) {
				std::swap(this->agentWritesIncoming, this->agentWritesBack);
				this->agentWritesNew = false;
				newData = true;
			}
			xSemaphoreGive(this->agentWritesMutex);
		}

		if(newData) {
			this->registers.at(RegisterType::Torque).value = this->agentWritesBack.torque;
			this->registers.at(RegisterType::AgentControlFrequency).value = this->agentWritesBack.agentFrequency;
			this->registers.at(RegisterType::AgentLocalHistorySize).value = this->agentWritesBack.localHistorySize;
			this->registers.at(RegisterType::AgentTraining).value = this->agentWritesBack.isTraining;
			this->registers.at(RegisterType::AgentNoiseAmplitude).value = this->agentWritesBack.noiseAmplitude;
			this->registers.at(RegisterType::AgentAddProportional).value = this->agentWritesBack.addProportional;
			this->registers.at(RegisterType::AgentAddConstant).value = this->agentWritesBack.addConstant;
		}
	}
}

//----------
void
Registry::motorControlWrite(MotorControlWrites && motorControlWrites)
{
	// Write incoming control loop writes into the incoming buffer (previous data is overwritten)
	if(xSemaphoreTake(this->motorControlWritesMutex, waitTime)) {
		this->motorControlWritesIncoming = motorControlWrites;
		this->motorControlWritesNew = true;
		xSemaphoreGive(this->motorControlWritesMutex);
	}
}

//----------
void
Registry::motorControlRead(MotorControlReads & motorControlReads)
{
	
	if(xSemaphoreTake(this->motorControlReadsMutex, waitTime)) {
		motorControlReads = this->motorControlReads;

		// Also take the freshest data from agent writes
		if(xSemaphoreTake(this->agentWritesMutex, 1 / portTICK_PERIOD_MS)) {
			if(this->agentWritesNew) {
				motorControlReads.torque = this->agentWritesIncoming.torque;
			}
			xSemaphoreGive(this->agentWritesMutex);
		}
		xSemaphoreGive(this->motorControlReadsMutex);
	}
}

//----------
void
Registry::agentWrite(AgentWrites && agentWrites)
{
	// Write incoming control loop writes into the incoming buffer (previous data is overwritten)
	if(xSemaphoreTake(this->agentWritesMutex, waitTime)) {
		this->agentWritesIncoming = agentWrites;
		this->agentWritesNew = true;
		xSemaphoreGive(this->agentWritesMutex);
	}
}

//----------
void
Registry::agentRead(AgentReads & agentReads)
{
	
	if(xSemaphoreTake(this->agentReadsMutex, waitTime)) {
		agentReads = this->agentReads;

		// Also take the freshest data from motor drive writes
		if(xSemaphoreTake(this->motorControlWritesMutex, 1 / portTICK_PERIOD_MS)) {
			if(this->motorControlWritesNew) {
				// The data in motorControlWritesIncoming is fresher than the registry, so use it♦
				agentReads.multiTurnPosition = this->motorControlWritesIncoming.multiTurnPosition;
				agentReads.velocity = this->motorControlWritesIncoming.velocity;
				agentReads.motorControlFrequency = this->motorControlWritesIncoming.motorControlFrequency;
			}
			xSemaphoreGive(this->motorControlWritesMutex);
		}


		xSemaphoreGive(this->agentReadsMutex);
	}
}

//----------
void
Registry::loadDefaults()
{
	auto file = fopen(registerDefaultsFilename, "rb");
	if(!file) {
		printf("[Registry] Cannot open file for reading (%s)\n", registerDefaultsFilename);
		return;
	}

	// Get file size and check
	fseek(file, 0L, SEEK_END);
	auto fileSize = ftell(file);
	rewind(file);

	auto count = fileSize / (sizeof(RegisterType) + sizeof(int32_t));
	RegisterType registerType;
	for(int i=0; i<count; i++) {
		fread(&registerType, sizeof(RegisterType), 1, file);
		fread(&registers.at(registerType).value, sizeof(int32_t), 1, file);
	}

	fclose(file);
}

//----------
void
Registry::saveDefault(const RegisterType & registerType)
{
	this->defaultsToSave.insert(registerType);

	auto file = fopen(registerDefaultsFilename, "wb");
	if(!file) {
		printf("[Registry] Cannot open file for saving (%s)", registerDefaultsFilename);
		return;
	}

	for(const auto & regsiterTypeToSave : this->defaultsToSave) {
		fwrite(&regsiterTypeToSave, sizeof(RegisterType), 1, file);
		fwrite(&registers.at(regsiterTypeToSave).value, sizeof(int32_t), 1, file);
	}

	fclose(file);
}