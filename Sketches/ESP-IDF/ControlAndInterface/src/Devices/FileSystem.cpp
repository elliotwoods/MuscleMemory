#include "FileSystem.h"
#include "GUI/Controller.h"

//----------
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

namespace Devices {
	//----------
	void
	FileSystem::listPartitions()
	{
		printf("App partitions:");
		partloop(ESP_PARTITION_TYPE_APP);
		printf("Data partitions:");
		partloop(ESP_PARTITION_TYPE_DATA);
	}
	
	//----------
	FileSystem::~FileSystem()
	{
		this->unmount();
	}

	//----------
	bool
	FileSystem::mount(const char * partitionLabel, const char * mountPoint, bool formatIfNeeded, uint8_t maxOpenFiles)
	{
		if(this->wl_handle != WL_INVALID_HANDLE) {
			printf("Already mounted %s at %s\n", this->partitionLabel.c_str(), this->mountPoint.c_str());
			return true;
		}

		auto partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, partitionLabel);

		if(!partition) {
			printf("No FAT patition found\n");
			return false;
		}

		if(GUI::Controller::X().isDialButtonPressed()) {
			printf("Erasing FAT partition\n");
			esp_partition_erase_range(partition, 0, partition->size);
		}

		esp_vfs_fat_mount_config_t config = {
			.format_if_mount_failed = formatIfNeeded,
			.max_files = maxOpenFiles,
			.allocation_unit_size = CONFIG_WL_SECTOR_SIZE
		};

		{
			auto error = esp_vfs_fat_spiflash_mount(mountPoint
				, partitionLabel
				, &config
				, &wl_handle);
			if(error) {
				printf("Could not mount FAT partition, error : %d\n", error);
				esp_vfs_fat_spiflash_unmount(mountPoint, this->wl_handle);
				this->wl_handle = WL_INVALID_HANDLE;
				return false;
			}

			this->partitionLabel = std::string(partitionLabel);
			this->mountPoint = std::string(mountPoint);
		}

		return true;
	}

	//----------
	void
	FileSystem::unmount()
	{
		esp_vfs_fat_spiflash_unmount(this->mountPoint.c_str(), this->wl_handle);

		this->wl_handle = WL_INVALID_HANDLE;
		this->partitionLabel.clear();
		this->mountPoint.clear();
	}
}