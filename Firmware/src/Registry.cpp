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
Registry::Register::Register(const Register & other)
: name(other.name)
#ifdef ATOMIC_REGISTERS
, value(other.value.load())
#else
, value(other.value)
#endif
, defaultValue(other.defaultValue)
, access(other.access)
, range(other.range)
{

}

//----------
Registry::Register::Register(const std::string & name, int32_t value, Access access)
: name(name)
, value(value)
, defaultValue(value)
, access(access)
, range({false})
{

}

//----------
Registry::Register::Register(const std::string & name, int32_t value, Access access, int32_t min, int32_t max)
: name(name)
, value(value)
, defaultValue(value)
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
	this->frameTimer.init();
}

//----------
Registry::~Registry()
{
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
		fread(&registers.at(registerType).defaultValue, sizeof(int32_t), 1, file);
		setRegisterValue(registerType, registers.at(registerType).defaultValue);

		// Remember that this register default needs saving
		this->defaultsToSave.insert(registerType);
	}

	fclose(file);
}

//----------
void
Registry::saveDefault(const RegisterType & registerType)
{
	// Set the cached default value
	registers.at(registerType).defaultValue = getRegisterValue(registerType);

	// Remember to save this default value
	this->defaultsToSave.insert(registerType);

	// Save all default values (not just this one)
	auto file = fopen(registerDefaultsFilename, "wb");
	if(!file) {
		printf("[Registry] Cannot open file for saving (%s)", registerDefaultsFilename);
		return;
	}

	for(const auto & regsiterTypeToSave : this->defaultsToSave) {
		fwrite(&regsiterTypeToSave, sizeof(RegisterType), 1, file);
		fwrite(&registers.at(regsiterTypeToSave).defaultValue, sizeof(int32_t), 1, file);
	}

	fclose(file);
}

#ifdef ATOMIC_REGISTERS
//----------
int32_t IRAM_ATTR
getRegisterValue(const Registry::RegisterType & registerType)
{
	static const auto & registry = Registry::X();
	return registry.registers.at(registerType).value.load();
}
#else 
//----------
const int32_t & IRAM_ATTR
getRegisterValue(const Registry::RegisterType & registerType)
{
	static const auto & registry = Registry::X();
	return registry.registers.at(registerType).value;
}
#endif

//----------
const Registry::Range & IRAM_ATTR
getRegisterRange(const Registry::RegisterType & registerType)
{
	static const auto & registry = Registry::X();
	return registry.registers.at(registerType).range;
}


//----------
void IRAM_ATTR
setRegisterValue(const Registry::RegisterType & registerType, int32_t value)
{
	static auto & registry = Registry::X();
#ifdef ATOMIC_REGISTERS
	registry.registers.at(registerType).value.store(value, std::memory_order_relaxed);
#else
	registry.registers.at(registerType).value = value;
#endif
}