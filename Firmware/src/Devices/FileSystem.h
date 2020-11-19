#pragma once

#include <string>
#include "esp_vfs_fat.h"

namespace Devices {
	class FileSystem {
	public:
		static void listPartitions();

		~FileSystem();
		bool mount(const char * partitionLabel, const char * mountPoint, bool formatIfNeeded, uint8_t maxOpenFiles = 8);
		void unmount();

		void format();
	private:
		wl_handle_t wl_handle = WL_INVALID_HANDLE;
		const esp_partition_t * partition = nullptr;
		std::string partitionLabel;
		std::string mountPoint;
	};
}
