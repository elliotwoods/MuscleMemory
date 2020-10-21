#include "MultiTurn.h"

void saveTaskMethod(void* multiTurnUntyped)
{
	auto & multiTurn = * (Control::MultiTurn*) multiTurnUntyped;
	auto & saveData = multiTurn.lastSave;

	while(true) {
		vTaskSuspend(NULL);
		auto position = multiTurn.getMultiTurnPosition();
		if(abs(position) - saveData.multiTurnPosition > 1 << 12) {
			saveData.fileIndex ^= 1;
			saveData.multiTurnPosition = position;
			saveData.saveSequenceIndex++;

			// Create the checksum
			{
				uint16_t crc = 0;
				for(size_t i=0; i<sizeof(Control::MultiTurn::SaveData) - sizeof(uint16_t); i++) {
					crc += ((uint8_t*) &saveData)[i];
				}
				saveData.checkSum = crc;
			}

			// Open the file for saving
			char filename[100];
			sprintf(filename, "MultiTurn%d.dat", saveData.fileIndex);
			auto file = fopen(filename, "wb");

			fwrite(&saveData, sizeof(Control::MultiTurn::SaveData), 1, file);
			fclose(file);

			printf("Saving %s\n", filename);
		}
	}
}

#define HALF_WAY (1 << 13)
namespace Control {
	//-----------
	MultiTurn::MultiTurn(EncoderCalibration & encoderCalibration)
	: encoderCalibration(encoderCalibration)
	{
		xTaskCreatePinnedToCore(saveTaskMethod
			, "MultiTurn save"
			, 1024
			, this
			, 5
			, &this->saveTaskHandle
			, 0);
	}

	//-----------
	MultiTurn::~MultiTurn()
	{
		vTaskDelete(this->saveTaskHandle);
	}

	//-----------
	void
	MultiTurn::init(SingleTurnPosition singleTurnPosition)
	{
		this->priorSingleTurnPosition = singleTurnPosition;
		this->position = this->priorSingleTurnPosition;
		this->turns = 0;

		// Load the multiturn session
		this->loadSession(singleTurnPosition);
	}

	//-----------
	void
	MultiTurn::update(SingleTurnPosition currentSingleTurnPosition)
	{
		if(this->priorSingleTurnPosition > HALF_WAY / 4 * 3 && currentSingleTurnPosition < HALF_WAY / 4)
		{
			this->turns++;
		}
		else if(this->priorSingleTurnPosition < HALF_WAY / 4 && currentSingleTurnPosition > HALF_WAY / 4 * 3)
		{
			this->turns--;
		}
		this->position = (((int32_t) this->turns) << 14) + (int32_t) currentSingleTurnPosition;
		this->priorSingleTurnPosition = currentSingleTurnPosition;
		vTaskResume(this->saveTaskHandle);
	}

	//-----------
	int32_t
	MultiTurn::getMultiTurnPosition() const
	{
		return this->position;
	}

	//-----------
	bool
	MultiTurn::loadSession(SingleTurnPosition currentSingleTurn)
	{
		// We keep 2 files in case one becomes corrupted

		SaveData A, B;
		auto Aloaded = this->loadSessionFile("MultiTurn0.dat", A);
		auto Bloaded = this->loadSessionFile("MultiTurn1.dat", B);

		SaveData * saveData = nullptr;
		if(Aloaded && Bloaded) {
			saveData = A.saveSequenceIndex > B.saveSequenceIndex
				? &A
				: &B;
		}
		else if(Aloaded) {
			saveData = &A;
		}
		else if(Bloaded) {
			saveData = &B;
		}
		else {
			return false;
		}

		auto turns = saveData->multiTurnPosition >> 14;
		auto savedSingleTurn = saveData->multiTurnPosition % (1 << 14);
		if(savedSingleTurn - currentSingleTurn > 1 << 13) {
			// meanwhile we've skipped a cycle negative
			turns--;
		}
		else if(currentSingleTurn - savedSingleTurn > 1 << 13) {
			// meanwhile we've skipped a cycle positive
			turns++;
		}
		this->turns = turns;
		this->lastSave = * saveData;

		return true;
	}

	//-----------
	bool
	MultiTurn::loadSessionFile(const char * filename, SaveData & saveData)
	{
		// Load the file
		auto file = fopen(filename, "rb");
		if(!file) {
			return false;
		}

		// Get file size and check
		fseek(file, 0L, SEEK_END);
		auto size = ftell(file);
		rewind(file);
		if(size != sizeof(SaveData)) {
			return false;
		}

		fread(&saveData, sizeof(SaveData), 1, file);
		fclose(file);

		// Check checksum
		{
			uint16_t crc = 0;
			for(size_t i=0; i<sizeof(SaveData) - sizeof(uint16_t); i++) {
				crc += ((uint8_t*) &saveData)[i];
			}
			if(crc != saveData.checkSum) {
				printf("[MultiTurn] Checksum failed loading %s\n", filename);
				return false;
			}	
		}

		return true;
	}
}