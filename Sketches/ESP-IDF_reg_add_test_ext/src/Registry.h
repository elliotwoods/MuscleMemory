#include <map>
#include <string>

class Registry {
public:
	enum RegisterType : uint16_t {
		CurrentPosition = 0,
		CurrentVelocity = 1,
		TargetPosition = 2,
		TargetVelocity = 3,

		CurrentI = 10,
		MaximumI = 11,
		CurrentVBus = 12
	};

	struct Register {
		std::string name;
		int32_t value;
	};

	std::map<RegisterType, Register> registers {
		{ RegisterType::CurrentPosition, {
			"CurrentPosition"
			, 30
		}},
		{ RegisterType::CurrentVelocity, {
			"CurrentVelocity"
			, 0
		}},
		{ RegisterType::TargetPosition, {
			"TargetPosition"
			, 0
		}},
		{ RegisterType::TargetVelocity, {
			"TargetVelocity"
			, 0
		}},
		{ RegisterType::CurrentI, {
			"CurrentI"
			, 0
		}},
		{ RegisterType::MaximumI, {
			"MaximumI"
			, 0
		}},
		{ RegisterType::CurrentVBus, {
			"CurrentVBus"
			, 0
		}}
	};

    static Registry & X();
private:
    Registry() { }
};