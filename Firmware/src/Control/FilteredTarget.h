#pragma once

#include "DataTypes.h"

namespace Control {
	class FilteredTarget {
	public:
		static FilteredTarget & X();
		void update(); // called from main loop
		void notifyTargetChange();
		MultiTurnPosition getTargetFiltered() const; // called from drive loop
	private:
		FilteredTarget();
		bool targetChange = true;
		Velocity filterVelocity = 0;
		MultiTurnPosition priorTarget = 0;
		uint64_t priorTargetTimestamp = 0;
		const uint32_t maxTimeDelta = 1 * 1000000; // 1 second
	};
}