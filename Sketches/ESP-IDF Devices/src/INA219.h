// referencining https://github.com/johngineer/ArduinoINA219/blob/master/INA219.cpp

#include "stdint.h"

class INA219 {
public:
    enum class Register {
        Configuration = 0x00
        , ShuntVoltage = 0x01
        , BusVoltage = 0x02
        , Power = 0x03
        , Current = 0x04
        , Calibration = 0x05
    };

    struct Configuration {
        enum VoltageRange : uint8_t {
            From_0_to_16V = 0
            , From_0_to_32V = 1
        };

        enum Gain : uint8_t {
            Gain_1_Range_40mV = 0
            , Gain_2_Range_80mV = 1
            , Gain_4_Range_160mV = 2
            , Gain_8_Range_320mV = 3

            , Gain_Auto = 8
        };

        enum ADCResolution : uint8_t {
            Resolution9bit_84us = 0
            , Resolution10bit_148us = 1
            , Resolution11bit_276us = 2
            , Resolution12bit_532us = 3
            , Resolution12bit_532us_2 = 8
            
            , Samples2_1_06ms = 9 // 1.06ms, etc
            , Samples4_2_13ms = 10
            , Samples8_4_26ms = 11
            , Samples16_8_51ms = 12
            , Samples32_17_02ms = 13
            , Samples64_34_05ms = 14
            , Samples128_68_10ms = 15
        };

        enum OperatingMode : uint8_t {
            PowerDown = 0
            , ShuntVoltageTriggered = 1
            , BusVoltageTriggered = 2
            , ShuntAndBusTriggered = 3
            
            , ADCOff = 4

            , ShuntVoltageContinuous = 5
            , BusVoltageContinuous = 6
            , ShuntAndBusContinuous = 7
        };

        VoltageRange voltageRange = VoltageRange::From_0_to_32V;
        Gain gain = Gain::Gain_Auto;
        ADCResolution currentResolution = ADCResolution::Resolution12bit_532us;
        ADCResolution busVoltageResolution = ADCResolution::Resolution12bit_532us;
        OperatingMode operatingMode = OperatingMode::ShuntAndBusContinuous;

        // Shunt resistor value in Ohms
        float shuntValue;

        /// Maximum expected current in Amps. This is used to calculate the current LSB readback and gain values
        float maximumCurrent;
    };

    enum Errors : uint8_t {
        Overflow
        , NoAcknowledge
    };

    INA219();
    void init(const Configuration &, uint8_t address = 0b1000000);
    float getCurrent();
    float getPower();
    float getBusVoltage();
private:
    uint16_t readRegister(Register);
    void writeRegister(Register, uint16_t value);
    
    void reset();
    void setConfiguration();
    void setCalibration();

    void calculateGain();
    Configuration configuration;

    uint8_t errors = 0;

    // A cached value
    float lsbToCurrent;
};
