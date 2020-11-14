#pragma once

#include "Stream.h"
#include "GUI/Panels/OTADownload.h"
#include <memory>

extern "C" {
	#include "esp_ota_ops.h"
}

namespace Utils
{
	class OTAStream : public Stream
	{
	public:
		void begin(size_t size);

		size_t write(uint8_t) override;
		size_t write(const uint8_t *buffer, size_t size) override;

		void end();

		int available() override { return 0; }
		int read() override { return 0; }
		int peek() override { return 0; }
		void flush() override { }
	private:
		const esp_partition_t * otaPartition;
		esp_ota_handle_t otaHandle;
		std::shared_ptr<GUI::Panels::OTADownload> panel;
	};
}