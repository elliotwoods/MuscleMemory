#include "Dashboard.h"
#include "Registry.h"

#define OUTER_CIRCLE
#define HAC_DRAW_TICKS
#define MULTI_TURN
#define SINGLE_TURN

const u8g2_uint_t centerCircle[2] = {32, 32};
const float markerRadiusMajor = 23;
const float markerRadiusMinor = 18;
const float notchRadiusOuter = 32.0f;

namespace GUI {
	namespace Panels {
		//----------
		Dashboard::Dashboard()
		{

		}

		//----------
		void
		Dashboard::update()
		{
			
		}

		//----------
		void
		Dashboard::draw(U8G2& u8g2)
		{
			auto MultiTurnPosition = Registry::X().registers.at(Registry::RegisterType::MultiTurnPosition); 
			auto TargetPosition = Registry::X().registers.at(Registry::RegisterType::TargetPosition); 
			#ifdef OUTER_CIRCLE
				u8g2.drawCircle(centerCircle[0], centerCircle[1], notchRadiusOuter, U8G2_DRAW_ALL);
			#endif
			#ifdef HAC_DRAW_TICKS
				//draw clock notches
				{
					const uint8_t innerNotches[24] PROGMEM = { 32, 6, 46, 7, 57, 17, 58, 32, 57, 46, 47, 57, 32, 58, 18, 57, 7, 47, 6, 32, 7, 18, 17, 7 };
					const uint8_t outerNotches[24] PROGMEM = { 32, 1, 48, 5, 59, 16, 63, 32, 59, 47, 48, 59, 32, 63, 17, 59, 5, 48, 1, 32, 5, 17, 16, 5 };

					for(int i=0; i<12; i++) {
						u8g2.drawLine(innerNotches[i * 2 + 0], innerNotches[i * 2 + 1], outerNotches[i * 2 + 0], outerNotches[i * 2 + 1]);
					}
				}
			#endif
			#ifdef MULTI_TURN
			float MultiTurnFloat;
			// MultiTurnPosition - actual
			{
				auto value = MultiTurnPosition.value / (1 << 14);
				float phase = float(value) / 1024.0f * TWO_PI;
				u8g2_uint_t x = centerCircle[0] + markerRadiusMajor * sinf(phase);
				u8g2_uint_t y = centerCircle[1] - markerRadiusMajor * cosf(phase);
				u8g2.drawDisc(x, y, 3, U8G2_DRAW_ALL);
				MultiTurnFloat = phase;
			}
			// Target multi-turn position
			{
				auto value = TargetPosition.value / (1 << 14);
				float phase = float(value) / 1024.0f * TWO_PI;
				u8g2_uint_t x = centerCircle[0] + markerRadiusMajor * sinf(phase);
				u8g2_uint_t y = centerCircle[1] - markerRadiusMajor * cosf(phase);
				u8g2.drawCircle(x, y, 4, U8G2_DRAW_ALL);
			}
			#endif
			#ifdef SINGLE_TURN
				int16_t SingleTurn = MultiTurnPosition.value % (1<<14);
				float SinglePhase = float(SingleTurn) * TWO_PI / float(1<<14);
				u8g2_uint_t single_x = centerCircle[0] + markerRadiusMinor * sinf(SinglePhase);
				u8g2_uint_t single_y = centerCircle[1] - markerRadiusMinor * cosf(SinglePhase);
				u8g2.drawPixel(single_x, single_y);
			#endif
			{
				u8g2.setFont(u8g2_font_nerhoe_tr);
				char message[200];
				auto BusVoltage = Registry::X().registers.at(Registry::RegisterType::BusVoltage);
				auto Current = Registry::X().registers.at(Registry::RegisterType::Current);

				sprintf(message, "%.2f", (float)BusVoltage.value/1000);			
				u8g2.drawStr(75, 15, message);
				sprintf(message, "V.");			
				u8g2.drawStr(105, 15, message);

				sprintf(message, "%.2f", (float)Current.value);			
				u8g2.drawStr(75, 27, message);
				sprintf(message, "A.");			
				u8g2.drawStr(105, 27, message);
				
				sprintf(message, "MultiTurn");
				u8g2.drawStr(75, 45, message);
				sprintf(message, "%.3f", MultiTurnFloat);			
				u8g2.drawStr(75, 57, message);
			}

			//printf("%d,%f - %d,%f \n",MultiTurn,MultiPhase,SingleTurn,SinglePhase);
			/*
			auto deviceID = Registry::X().registers.at(Registry::RegisterType::DeviceID);

			u8g2.setFont(u8g2_font_nerhoe_tr);
			char message[200];
			sprintf(message, "%d", deviceID.value);			
			u8g2.drawStr(70, 37, message);
			*/
		}

		
		//----------
		bool
		Dashboard::buttonPressed()
		{
			return false;
		}

		
		//----------
		void
		Dashboard::dial(int8_t)
		{
			
		}
	}
}


