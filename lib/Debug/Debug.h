#pragma once

#include <cstdint>
#include <libopencm3/stm32/rcc.h>

class Debug {
public:
    struct Config {
        uint32_t usart;
        uint32_t gpio_port;
        uint16_t gpio_pins;
        uint8_t gpio_af;
    };

    struct Data {
        uint32_t tip_temp;
        uint32_t set_temp;
        uint32_t power;
    };


    Debug(const Config& config);
    void sendData(Data& data);

private:
    uint32_t m_usart;
};