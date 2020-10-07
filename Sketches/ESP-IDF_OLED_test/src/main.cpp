#include "Arduino.h"
#include <driver/gpio.h>
#include "U8g2lib.h"

extern "C"
{
#include <u8g2_esp32_hal.h>
}

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, 15, 4, 16);

void setup()
{
	Serial.begin(115200);
	Serial.println("ssss0");
    
	u8g2.begin();	
}

int xOff = 24;
void draw() {
	u8g2.drawCircle(64,32,20);
	u8g2.drawLine(64,32,64,15);
	u8g2.setFont(u8g2_font_ncenB14_tr);
	u8g2.drawStr(0,xOff,"Hello World!");
	xOff++;
	if(xOff>100){
		xOff = 0;
	}
}

void loop()
{

	u8g2.firstPage();
	do {
		draw();
	} while (u8g2.nextPage());

}



class Panel {
public:
	virtual void draw();
	virtual void up();
	virtual void down();
	virtual void enter();
};

class IntegerDial {
public:
	void draw() override;
	void up() override;
	void down() override;
	void enter() override {
		panelManager.up();
	}
};

class PanelManager {
public:
private:
	std::queue<Panel*> displayStack;
};