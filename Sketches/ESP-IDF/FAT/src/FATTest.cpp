#include "FATTest.h"
#include "FFat.h"
#include "esp_vfs_fat.h"
#include "stdio.h"

const char partitionName[] = "appfs";

// reference : https://github.com/espressif/arduino-esp32/blob/master/libraries/FFat/src/FFat.cpp

wl_handle_t wl_handle = WL_INVALID_HANDLE;
std::string mountPoint;

const esp_partition_t *check_ffat_partition(const char* label)
{
	const esp_partition_t* ck_part = esp_partition_find_first(
	   ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, label);
	if (!ck_part) {
		log_e("No FAT partition found with label %s", label);
		return NULL;
	}
	return ck_part;
}

bool
mount(bool formatOnFail, const char * basePath, uint8_t maxOpenFiles, const char * partitionLabel)
{
	if(wl_handle != WL_INVALID_HANDLE) {
		printf("Already mounted\n");
		return true;
	}

	if(!check_ffat_partition(partitionLabel)) {
		printf("No FAT patition found\n");
		return false;
	}

	esp_vfs_fat_mount_config_t config = {
		.format_if_mount_failed = formatOnFail,
		.max_files = maxOpenFiles,
		.allocation_unit_size = CONFIG_WL_SECTOR_SIZE
	};
	{
		auto error = esp_vfs_fat_spiflash_mount(basePath
			, partitionLabel
			, &config
			, &wl_handle);
		if(error) {
			printf("Could not mount FAT partition, error : %d\n", error);
			esp_vfs_fat_spiflash_unmount(basePath, wl_handle);
			wl_handle = WL_INVALID_HANDLE;
			return false;
		}
		mountPoint = std::string(basePath);
	}
	return true;
}

void
unmount()
{
	if(wl_handle != WL_INVALID_HANDLE){
		esp_err_t err = esp_vfs_fat_spiflash_unmount(mountPoint.c_str(), wl_handle);
		if(err){
			log_e("Unmounting FFat partition failed! Error: %d", err);
			return;
		}
		wl_handle = WL_INVALID_HANDLE;
		mountPoint.clear();
	}	
}

void partloop(esp_partition_type_t part_type) {
	esp_partition_iterator_t iterator = NULL;
	const esp_partition_t *next_partition = NULL;
	iterator = esp_partition_find(part_type, ESP_PARTITION_SUBTYPE_ANY, NULL);
	while (iterator) {
		next_partition = esp_partition_get(iterator);
		if (next_partition != NULL) {
			printf("partition addr: 0x%06x; size: 0x%06x; label: %s\n", next_partition->address, next_partition->size, next_partition->label);  
	 		iterator = esp_partition_next(iterator);
		}
	}
}

void
fatTest()
{
	printf("App partitions:");
	partloop(ESP_PARTITION_TYPE_APP);
	printf("Data partitions:");
	partloop(ESP_PARTITION_TYPE_DATA);

	const char filePath[] = "/appdata/test.txt";

	mount(true, "/appdata", 16, "appdata");

	auto fileExists = access(filePath, F_OK) != -1;
	if(fileExists) {
		printf("File exists! Printing file contents\n");

		FILE * file;
		if((file = fopen(filePath, "r"))) {
			fseek(file, 0, SEEK_END);
			auto fileSize = ftell(file);
			rewind(file);

			std::string fileContents;
			fileContents.resize(fileSize);
			printf("File length: %ld\n", fileSize);

			fread(&fileContents[0]
				, 1
				, fileSize
				, file);
			printf("%s\n", cfileContents.c_str());
			fclose(file);
		}
		else {
			printf("Failed to open file\n");
		}
	}
	else {
		printf("File doesn't exist, writing new file\n");

		FILE * file;
		if((file = fopen(filePath, "w"))) {
			std::string fileContents = "HERE ARE SOME CONTENTS INSIDE THE FILE";
			fwrite(fileContents.c_str(), sizeof(char), fileContents.size(), file);
			fclose(file);
		}
		else {
			printf("Failed to open file for writing\n");
		}
	}
}