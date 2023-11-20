#pragma once

#include <cstdint>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>

class AcControl {
public:
    AcControl(uint32_t timer, tim_oc_id timer_oc, rcc_periph_clken timer_rcc);

    void turnOn(uint32_t on_halfcycles);
    void turnOff();
    void zeroCrossingCallback();

private:
    uint32_t m_timer;
    tim_oc_id m_timer_oc;
    volatile uint32_t m_remaining_halfcycles = 0;
};