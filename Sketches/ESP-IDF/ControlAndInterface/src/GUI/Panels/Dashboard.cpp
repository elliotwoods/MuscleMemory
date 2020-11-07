#include "Dashboard.h"
#include "Registry.h"

#include "GUI/Controller.h"
#include "RegisterList.h"

#define OUTER_CIRCLE
#define HAC_DRAW_TICKS
#define MULTI_TURN
#define SINGLE_TURN

const u8g2_uint_t centerCircle[2] = {32, 32};
const float markerRadiusMajor = 23;
const float markerRadiusMinor = 18;
const float notchRadiusOuter = 32.0f;
const float rotationRange = 100 * 72 / 14;

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
			static auto & registry = Registry::X();
			const auto & DeviceID = registry.registers.at(Registry::RegisterType::DeviceID); 
			const auto & MultiTurnPosition = registry.registers.at(Registry::RegisterType::MultiTurnPosition); 
			const auto & TargetPosition = registry.registers.at(Registry::RegisterType::TargetPosition); 
			const auto & CanRxThisFrame = registry.registers.at(Registry::RegisterType::CANRxThisFrame); 
			const auto & CanTxThisFrame = registry.registers.at(Registry::RegisterType::CANTxThisFrame); 
			const auto & CanErrorsThisFrame = registry.registers.at(Registry::RegisterType::CANErrorsThisFrame); 
			

			// Outer circle
			u8g2.drawCircle(centerCircle[0], centerCircle[1], notchRadiusOuter, U8G2_DRAW_ALL);
			
			// Clock notches
			{
				const uint8_t innerNotches[24] PROGMEM = { 32, 6, 46, 7, 57, 17, 58, 32, 57, 46, 47, 57, 32, 58, 18, 57, 7, 47, 6, 32, 7, 18, 17, 7 };
				const uint8_t outerNotches[24] PROGMEM = { 32, 1, 48, 5, 59, 16, 63, 32, 59, 47, 48, 59, 32, 63, 17, 59, 5, 48, 1, 32, 5, 17, 16, 5 };

				for(int i=0; i<12; i++) {
					u8g2.drawLine(innerNotches[i * 2 + 0], innerNotches[i * 2 + 1], outerNotches[i * 2 + 0], outerNotches[i * 2 + 1]);
				}
			}

			// Draw soft limits
			{
				{
					auto registerValue = Registry::X().registers.at(Registry::RegisterType::SoftLimitMin).value;
					float phase = float(registerValue / (1 << 14)) / rotationRange * TWO_PI;

					u8g2.drawLine(centerCircle[0], centerCircle[1]
						, centerCircle[0] + markerRadiusMajor * sinf(phase)
						, centerCircle[0] - markerRadiusMajor * cosf(phase));
				}

				{
					auto registerValue = Registry::X().registers.at(Registry::RegisterType::SoftLimitMax).value;
					float phase = float(registerValue / (1 << 14)) / rotationRange * TWO_PI;
					u8g2.drawLine(centerCircle[0], centerCircle[1]
						, centerCircle[0] + markerRadiusMajor * sinf(phase)
						, centerCircle[0] - markerRadiusMajor * cosf(phase));
				}
			}

			// Inner solid circle
			{
				u8g2.drawDisc(centerCircle[0], centerCircle[1], 12);
			}

			char message[200];

			// Device ID
			{
				u8g2.setFont(u8g2_font_t0_18_mn);
				u8g2.setDrawColor(0);
				sprintf(message, "%d", DeviceID.value);
				u8g2.drawStr(centerCircle[0] - strlen(message) * 4, centerCircle[1] + 7, message);
				u8g2.setDrawColor(1);
			}

			// MultiTurnPosition - actual
			{
				
				auto value = MultiTurnPosition.value / (1 << 14);
				float phase = float(value) / rotationRange * TWO_PI;
				u8g2_uint_t x = centerCircle[0] + markerRadiusMajor * sinf(phase);
				u8g2_uint_t y = centerCircle[1] - markerRadiusMajor * cosf(phase);
				u8g2.drawDisc(x, y, 3, U8G2_DRAW_ALL);

				u8g2.setFont(u8g2_font_profont22_mn);
				sprintf(message, "%.2f", float(MultiTurnPosition.value) / float(1 << 14));			
				u8g2.drawStr(128 - strlen(message) * 12, 45, message);
			}

			// Target multi-turn position
			{
				auto value = TargetPosition.value / (1 << 14);
				float phase = float(value) / rotationRange * TWO_PI;
				u8g2_uint_t x = centerCircle[0] + markerRadiusMajor * sinf(phase);
				u8g2_uint_t y = centerCircle[1] - markerRadiusMajor * cosf(phase);
				u8g2.drawCircle(x, y, 4, U8G2_DRAW_ALL);

				u8g2.setFont(u8g2_font_t0_18_mn);
				sprintf(message, "%.2f", float(TargetPosition.value) / float(1 << 14));			
				u8g2.drawStr(128 - strlen(message) * 8 - 4, 63, message);
			}
			
			// Single turn
			{
				int16_t SingleTurn = MultiTurnPosition.value % (1<<14);
				float SinglePhase = float(SingleTurn) * TWO_PI / float(1<<14);
				u8g2_uint_t single_x = centerCircle[0] + markerRadiusMinor * sinf(SinglePhase);
				u8g2_uint_t single_y = centerCircle[1] - markerRadiusMinor * cosf(SinglePhase);
				u8g2.drawPixel(single_x, single_y);
			}

			// Right side text
			{
				u8g2.setFont(u8g2_font_nerhoe_tr);
				
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
				
				// show Current Position , Target Position	
				{
					if(CanRxThisFrame.value > 0) {
						u8g2.drawStr(0, 10, "Rx");
					}

					if(CanTxThisFrame.value > 0) {
						u8g2.drawStr(0, 64, "Tx");
					}

					if(CanErrorsThisFrame.value > 0) {
						u8g2.drawStr(0, 10, "ERR");
					}
				}
			}
		}

		
		//----------
		void
		Dashboard::buttonPressed()
		{
			Controller::X().setRootPanel(std::make_shared<RegisterList>());
		}

		
		//----------
		void
		Dashboard::dial(int8_t movement)
		{
			auto & targetPosition = Registry::X().registers.at(Registry::RegisterType::TargetPosition).value;
			targetPosition += 512 * (int32_t) movement;
		}
	}
}


