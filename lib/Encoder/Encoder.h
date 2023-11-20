#pragma once
#include <cstdint>

class Encoder {
public:
    struct Config {
        uint32_t timer;
        uint32_t port_a;
        uint16_t pin_a;
        uint32_t port_b;
        uint16_t pin_b;
        uint16_t gpio_af;
    };

    Encoder(Config& config);
    int32_t getDelta();

private:
    uint32_t m_timer;
    uint32_t m_port_a;
    uint16_t m_pin_a;
    uint32_t m_port_b;
    uint16_t m_pin_b;

    volatile uint32_t m_last = 0;
};