#pragma once

#include <cstdint>

class I2cDma {
public:
    struct Config {
        uint32_t i2c;
        uint32_t apb1_freq;
        uint32_t dma;
        uint8_t dma_channel;
    };

    I2cDma(Config& config);
    void startWrite(uint8_t i2c_addr, uint8_t data[], uint8_t count);
    bool isBusy();

private:
    uint32_t m_i2c;
    uint32_t m_dma;
    uint8_t m_dma_channel;
    volatile bool m_busy = false;

    void dmaIsr();
};