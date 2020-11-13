#include "OTADownload.h"

#include "Registry.h"

namespace GUI {
	namespace Panels {
		//----------
		void
		OTADownload::update()
		{

		}

		//----------
		void
		OTADownload::draw(U8G2 & u8g2)
		{
			// Draw title
			u8g2.setFont(u8g2_font_10x20_mr);
			u8g2.drawStr(5, 13, "OTA DOWNLOAD");

			char message[100];

			const auto position = getRegisterValue(Registry::RegisterType::OTAWritePosition);
			const auto size = getRegisterValue(Registry::RegisterType::OTASize);

			// Draw content
			{
				u8g2.setFont(u8g2_font_nerhoe_tr);
				sprintf(message, "%d", position);
				u8g2.drawStr(3, 26, message);
			}

			{
				u8g2.setFont(u8g2_font_nerhoe_tr);
				sprintf(message, "(Total : %d) ", size);
				u8g2.drawStr(3, 38, message);
			}

			// Progress bar
			if(size > 0) {
				u8g2.setDrawColor(2);
				u8g2.drawBox(0, 0, 128 * position / size, 64);
				u8g2.setDrawColor(1);
			}
		}

		//----------
		void
		OTADownload::buttonPressed()
		{

		}

		//----------
		void
		OTADownload::dial(int8_t)
		{

		}
	}
}
