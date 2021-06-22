#pragma once

#include "DataTypes.h"

namespace Control {
	class FilteredTarget {
	public:
		static FilteredTarget & X();
		void update(); // called from main loop
		void notifyTargetChange(int32_t targetPosition);
		MultiTurnPosition getTargetFiltered(); // called from drive loop
	private:
		FilteredTarget();
		MultiTurnPosition priorTarget = 0;
		uint64_t priorTargetTimestamp = 0;
		int64_t priorUpdateTimestamp = 0;
		const uint32_t maxTimeDelta = 200 * 1000; // 200ms
	};
}