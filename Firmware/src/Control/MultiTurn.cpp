#include "MultiTurn.h"
#include "GUI/Controller.h"

#include "esp_task_wdt.h"

#define MOD(a,b) ((((a)%(b))+(b))%(b))
#define HALF_WAY (1 << 13)

#define DEBUG_MULTITURN false

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

		// 'Mount' the partition
		this->partition = esp_partition_find_first( (esp_partition_type_t) 0x40,  (esp_partition_subtype_t) 0x00, "multiturn");
		if(!this->partition) {
			printf("[MultiTurn] Cannot mount MultiTurn partition");
			abort();
		}

		if(!GUI::Controller::X().isDialButtonPressed()) {
			// Load the multiturn session
			this->loadSession(singleTurnPosition);
		}
		else {
			this->formatPartition();
		}

		this->driveLoopUpdate(singleTurnPosition);
	}

	//-----------
	void IRAM_ATTR
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
					printf("[MultiTurn] Needs save! SaveData implied turn (%d) from saved multiturn (%d) and current single turn (%d), Actual turns (%d)\n"
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
	int32_t IRAM_ATTR
	MultiTurn::getMultiTurnPosition() const
	{
		return this->position;
	}

	//-----------
	void
	MultiTurn::saveSession()
	{
		this->saveData.multiTurnPosition = position;
		this->saveData.saveSequenceIndex++;

		// Create the checksum
		this->saveData.storedCRC = saveData.getCRC();

		// Align the size to 16 bytes
		auto saveDataSize = this->getSaveDataSize();

		auto writePosition = this->saveIndex * saveDataSize;

		// Check if we need to loop
		if(writePosition >= this->getWriteOffsetForLastEntry()) {
			this->saveIndex = 0;
			writePosition = 0;
		}

		// Format the sector if we're at the start of a sector. WARNING - REQUIRES ALL OTHER CORE FUNCTIONS TO BE IN IRAM OTHERWISE HALTS
		if(writePosition % 0x1000 == 0) {
			if(DEBUG_MULTITURN) {
				printf("[MultiTurn] Formatting (%d) bytes at sector (%d)\n", 0x1000, writePosition);
			}
			esp_partition_erase_range(this->partition
				, writePosition
				, 0x1000);
		}
		


		// Write the data
		if(DEBUG_MULTITURN) {
			printf("[MultiTurn] Saving multiturn (%d) at writePosition (%d)\n", this->saveData.multiTurnPosition, writePosition);
		}
		ESP_ERROR_CHECK(esp_partition_write(this->partition
			, writePosition
			, &this->saveData
			, saveDataSize));

		this->saveIndex++;
	}

	//-----------
	bool
	MultiTurn::loadSession(SingleTurnPosition currentSingleTurn)
	{
		// We keep 2 files in case one becomes corrupted

		SaveData freshestData;
		size_t freshestReadPosition;

		bool anyLoaded = false;

		// Align the size to 16 bytes
		size_t saveDataSize = this->getSaveDataSize();

		// Read first position, and if that fails, read last position
		{
			SaveData loadedData;
			esp_partition_read(this->partition
				, 0
				, &loadedData
				, saveDataSize);
			
			if(loadedData.getCRC() == loadedData.storedCRC) {
				if(DEBUG_MULTITURN) {
					printf("[MultiTurn] First entry is valid\n");
				}
				freshestData = loadedData;
				freshestReadPosition = 0;
				anyLoaded = true;
			}
			else {
				if(DEBUG_MULTITURN) {
					printf("[MultiTurn] First entry is not valid. Trying last one\n");
				}
				auto readPosition = this->getWriteOffsetForLastEntry();
				esp_partition_read(this->partition
					, readPosition
					, &loadedData
					, saveDataSize);

				if(loadedData.getCRC() == loadedData.storedCRC) {
					if(DEBUG_MULTITURN) {
						printf("[MultiTurn] Last entry is valid\n");
					}
					freshestData = loadedData;
					freshestReadPosition = readPosition;
					anyLoaded = true;
				}
				else {
					// Looks like we need to format
					printf("[MultiTurn] First and last entries are not valid\n");
				}
			}
		}

		// Read remaining positions if we got any data so far
		if(anyLoaded) {
			for(size_t readPosition=saveDataSize; readPosition<this->partition->size; readPosition += saveDataSize) {
				SaveData loadedData;
				esp_partition_read(this->partition
					, readPosition
					, &loadedData
					, saveDataSize);

				if(loadedData.getCRC() == loadedData.storedCRC) {
					if(DEBUG_MULTITURN) {
						printf("[MultiTurn] Entry %" PRIu32 " at readPosition (%" PRIu32 ") with saveSequenceIndex (%" PRIu64 ") and saved MultiTurn position (%" PRIi32 ") is valid\n"
							, readPosition / saveDataSize
							, readPosition
							, loadedData.saveSequenceIndex
							, loadedData.multiTurnPosition);
					}
					if(loadedData.saveSequenceIndex > freshestData.saveSequenceIndex) {
						freshestData = loadedData;
						freshestReadPosition = readPosition;
					}
					else {
						break;
					}
				}
				else {
					break;
				}
			}
		}

		if(anyLoaded) {
			this->saveData = freshestData;
			this->saveIndex = freshestReadPosition / saveDataSize + 1;
			this->turns = this->implyTurns(freshestData.multiTurnPosition, currentSingleTurn);

			if(DEBUG_MULTITURN) {
				printf("[MultiTurn] Loaded multiturn data, turns (%d)\n", turns);
			}
			return true;
		}
		else {
			printf("[MultiTurn] Failed to load any data. Formatting\n");
			this->formatPartition();
			return false;
		}
		
	}

	//-----------
	size_t
	MultiTurn::getSaveDataSize() const
	{
		auto saveDataSize = sizeof(SaveData);
		if(saveDataSize % 16 != 0) {
			saveDataSize = (saveDataSize / 16) * 16 + 16;
		}
		return saveDataSize;
	}

	//-----------
	size_t
	MultiTurn::getWriteOffsetForLastEntry() const
	{
		auto saveDataSize = this->getSaveDataSize();
		return ((this->partition->size / saveDataSize) - 1) * saveDataSize;
	}

	//-----------
	void
	MultiTurn::formatPartition()
	{
		if(DEBUG_MULTITURN) {
			printf("[MultiTurn] Formatting save data partition\n");
		}

		// We often get WDT reboots if we erase all at once, so try erasing in portions
		
		size_t offset = 0;
		const size_t eraseRegionSize = 0x10000;

		// Erase by portion
		for(; offset < this->partition->size; offset += eraseRegionSize) {
			if(DEBUG_MULTITURN) {
				printf("[MultiTurn] Erasing (%" PRIu32 ") bytes at (%" PRIu32 ")\n", eraseRegionSize, offset);
			}
			esp_partition_erase_range(this->partition
				, offset
				, eraseRegionSize);
			esp_task_wdt_reset();

			// Allow some time for other tasks
			vTaskDelay(1);
		}

		// Check if we ended exactly on the size
		if(offset != this->partition->size) {
			// Otherwise step backwards
			offset -= eraseRegionSize;

			// And erase the remaining region
			esp_partition_erase_range(this->partition
				, offset
				, this->partition->size - offset);
		}
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
}