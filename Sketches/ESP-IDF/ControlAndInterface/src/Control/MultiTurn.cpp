#include "MultiTurn.h"

#define MOD(a,b) ((((a)%(b))+(b))%(b))
#define HALF_WAY (1 << 13)

#define DEBUG_MULTITURN true

namespace Control {
	//-----------
	uint8_t
	MultiTurn::SaveData::getCRC() const
	{
		uint16_t crc = 0;
		auto data = (uint8_t*) this;

		// Calculate the CRC of this struct excluding the CRC itself
		while(data != &this->storedCRC)
		{
			crc += *data++;
		}
		return crc & 0xFF;
	}

	//-----------
	MultiTurn::MultiTurn(EncoderCalibration & encoderCalibration)
	: encoderCalibration(encoderCalibration)
	{
	}

	//-----------
	MultiTurn::~MultiTurn()
	{
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

		this->driveLoopUpdate(singleTurnPosition);
	}

	//-----------
	void
	MultiTurn::driveLoopUpdate(SingleTurnPosition currentSingleTurnPosition)
	{
		if(this->priorSingleTurnPosition > HALF_WAY / 2 * 3 && currentSingleTurnPosition < HALF_WAY / 2)
		{
			this->turns++;
		}
		else if(this->priorSingleTurnPosition < HALF_WAY / 2 && currentSingleTurnPosition > HALF_WAY / 2 * 3)
		{
			this->turns--;
		}
		this->position = (((int32_t) this->turns) << 14) + (int32_t) currentSingleTurnPosition;
		this->priorSingleTurnPosition = currentSingleTurnPosition;
	}
	
	//-----------
	void
	MultiTurn::mainLoopUpdate()
	{
		// If our saveData implies a different number of turns than our actual number of turns
		// (when loaded with our current encoder reading being active)
		{
			auto mtPosition = this->getMultiTurnPosition();
			auto stPosition = MOD(mtPosition, 1 << 14);
			auto closestTurn = this->implyTurns(this->saveData.multiTurnPosition, stPosition);
			if(closestTurn != this->turns) {
				if(DEBUG_MULTITURN) {
					printf("[MultiTurn] Saving! SaveData implied turn (%d) from saved pos (%d) and current single turn pos (%d), Actual turns (%d)\n"
						, closestTurn
						, this->saveData.multiTurnPosition
						, stPosition
						, this->turns);
				}
				

				// Then save the session
				this->saveSession();
			}
		}
	}

	//-----------
	int32_t
	MultiTurn::getMultiTurnPosition() const
	{
		return this->position;
	}

	//-----------
	void
	MultiTurn::saveSession()
	{
		this->saveData.fileIndex++;
		this->saveData.multiTurnPosition = position;
		this->saveData.saveSequenceIndex++;

		if(DEBUG_MULTITURN) {
			printf("[MultiTurn] Saving multiturn (%d)\n", this->saveData.multiTurnPosition);
		}

		// Create the checksum
		this->saveData.storedCRC = saveData.getCRC();

		// Open the file for saving
		char filename[100];
		MultiTurn::renderFileName(filename, saveData.fileIndex);
		if(DEBUG_MULTITURN) {
			printf("[MultiTurn] Opening file for save (%s)\n", filename);
		}
		auto file = fopen(filename, "wb");

		if(file) {
			printf("Saving %s\n", filename);
			fwrite(&this->saveData, sizeof(SaveData), 1, file);
			fclose(file);
		}
		else {
			printf("[MultiTurn] Couldn't open file for saving %s\n", filename);
		}
	}

	//-----------
	bool
	MultiTurn::loadSession(SingleTurnPosition currentSingleTurn)
	{
		// We keep 2 files in case one becomes corrupted

		SaveData freshestData;
		bool anyLoaded = false;
		for(uint8_t fileIndex=0; fileIndex<255; fileIndex++) {
			char filename[100];
			MultiTurn::renderFileName(filename, fileIndex);
			
			SaveData saveData;
			if(MultiTurn::loadSessionFile(filename, saveData)) {
				if(saveData.saveSequenceIndex > freshestData.saveSequenceIndex || !anyLoaded)
				{
					anyLoaded = true;
					freshestData = saveData;
				}
			}
		}

		if(!anyLoaded) {
			return false;
		}
		
		this->saveData = freshestData;
		this->turns = this->implyTurns(freshestData.multiTurnPosition, currentSingleTurn);

		printf("[MultiTurn] Loaded multiturn data (%d)\n", turns);
		return true;
	}

	//-----------
	Turns
	MultiTurn::implyTurns(MultiTurnPosition priorMultiTurnPosition, SingleTurnPosition currentSingleTurnPosition) const
	{
		auto turns = priorMultiTurnPosition >> 14;
		auto priorSingleTurn = MOD(priorMultiTurnPosition, 1 << 14);
		
		const auto halfWay = 1 << 13;
		const auto delta = (int)currentSingleTurnPosition - priorSingleTurn;
		if(delta > halfWay) {
			// meanwhile we've skipped forward a cycle
			turns--;
		}
		else if(-delta > halfWay) {
			// meanwhile we've skipped backwards a cycle
			turns++;
		}
		return turns;
	}

	//-----------
	void
	MultiTurn::renderFileName(char * filename, uint8_t index)
	{
		sprintf(filename, "/appdata/MultiTurn%d.dat", index);
	}

	//-----------
	bool
	MultiTurn::loadSessionFile(const char * filename, SaveData & loadedSaveData)
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

		fread(&loadedSaveData, sizeof(SaveData), 1, file);
		fclose(file);

		printf("[MultiTurn] File (%s), fileIndex (%d), saveSequenceIndex (%lld), multiTurnPosition (%d), crc (%d)\n"
			, filename
			, (int) loadedSaveData.fileIndex
			, loadedSaveData.saveSequenceIndex
			, loadedSaveData.multiTurnPosition
			, (int) loadedSaveData.storedCRC);

		// Check checksum
		{
			auto crc = loadedSaveData.getCRC();
			if(loadedSaveData.storedCRC != crc) {
				printf("[MultiTurn] Saved checksum (%d) doesn't match calculated checksum (%d). Failed loading %s\n"
					, loadedSaveData.storedCRC
					, crc
					, filename);
				printf("[MultiTurn] Position in save (%d)\n"
					, loadedSaveData.multiTurnPosition);
				return false;
			}
		}

		return true;
	}
}