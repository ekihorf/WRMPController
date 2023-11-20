#pragma once

#include <cstdint>

class TempSensor {
public:
    TempSensor(uint32_t adc, uint32_t channel, uint32_t amp_gain, uint32_t vref_mv);
    void startConversion();
    uint32_t getTemperature();
    void setOffset(uint32_t offset);

private:
    uint32_t m_adc;
    uint32_t m_channel;
    uint32_t m_offset = 0;
    uint32_t m_amp_gain;
    uint32_t m_vref_mv;
};