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

		if(cJSON_HasObjectItem(json, "add_proportional")) {
			auto jsonObject = cJSON_GetObjectItem(json, "add_proportional");
			if(cJSON_IsNumber(jsonObject)) {
				this->addProportional = (float) jsonObject->valuedouble;
			}
		}

		if(cJSON_HasObjectItem(json, "add_constant")) {
			auto jsonObject = cJSON_GetObjectItem(json, "add_constant");
			if(cJSON_IsNumber(jsonObject)) {
				this->addConstant = (float) jsonObject->valuedouble;
			}
		}
	}
}