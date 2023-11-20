#pragma once

#include <cstdint>

class TempSensor {
public:
    TempSensor(uint32_t adc, uint32_t channel);
    void startConversion();
    uint32_t getTemperature();

private:
    uint32_t m_adc;
    uint32_t m_channel;
};