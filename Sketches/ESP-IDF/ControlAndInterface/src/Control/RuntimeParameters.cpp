#include "RuntimeParameters.h"

namespace Control {
	//----------
	void
	RuntimeParameters::deserialize(cJSON * json)
	{
		if(cJSON_HasObjectItem(json, "is_training")) {
			auto jsonObject = cJSON_GetObjectItemCaseSensitive(json, "is_training");
			if(cJSON_IsBool(jsonObject)) {
				this->isTraining = jsonObject->valueint;
			}
		}

		if(cJSON_HasObjectItem(json, "noise_amplitude")) {
			auto jsonObject = cJSON_GetObjectItem(json, "noise_amplitude");
			if(cJSON_IsNumber(jsonObject)) {
				this->noiseAmplitude = (float) jsonObject->valuedouble;
			}
		}
	}
}