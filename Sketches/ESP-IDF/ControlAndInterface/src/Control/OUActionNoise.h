#pragma once

namespace Control {
	class OUActionNoise {
	public:
		void init(float mean, float standardDeviation, float theta=0.15, float dt=1e-3);
		void reset();
		float get();
	private:
		float mean = 0.0f;
		float standardDeviation = 2.0f;
		float theta = 0.15f;
		float dt = 1e-3;
		float previousValue = 0.0f;
	};
}