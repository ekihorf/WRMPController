#pragma once

#include <cstdint>
#include <libopencm3/stm32/timer.h>

class AcControl {
public:
    struct Config {
        uint32_t timer;
        tim_oc_id timer_oc;
        uint32_t timer_clock_freq;
        uint32_t zero_cross_port;
        uint16_t zero_cross_pin;
        uint32_t zero_cross_exti;
        uint32_t heater_port;
        uint16_t heater_pin;
        uint8_t heater_pin_af;
    };

    AcControl(Config& config);

    void turnOn(uint32_t on_halfcycles);
    void turnOff();

private:
    uint32_t m_timer;
    tim_oc_id m_timer_oc;
    uint32_t m_exti;
    volatile uint32_t m_remaining_halfcycles = 0;

    void extiIsr();
};