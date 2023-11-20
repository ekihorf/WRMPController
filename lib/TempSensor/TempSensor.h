#pragma once

#include <cstdint>
#include "Units.h"

class TempSensor {
public:
    struct Config {
        uint32_t adc;
        uint8_t channel;
        uint32_t amp_gain;
        Millivolts vref;
    };

    TempSensor(Config& config);
    void performConversion();
    Celsius getTemperature();
    void setOffset(Celsius offset);

private:
    uint32_t m_adc;
    uint8_t m_channel;
    Celsius m_offset{0};
    uint32_t m_amp_gain;
    Millivolts m_vref;
};