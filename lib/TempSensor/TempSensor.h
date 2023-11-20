#pragma once

#include <cstdint>
#include "Units.h"

class TempSensor {
public:
    struct Config {
        uint32_t adc;
        uint8_t channel;
        uint32_t amp_gain;
        Voltage vref;
    };

    TempSensor(const Config& config);
    void performConversion();
    Temperature getTemperature();
    void setOffset(Temperature offset);
    void setVref(Voltage vref);
    void setAmpGain(uint32_t amp_gain);

private:
    uint32_t m_adc;
    uint8_t m_channel;
    Temperature m_offset{0};
    uint32_t m_amp_gain;
    Voltage m_vref;
};