#pragma once

#include <cstdint>
#include <libopencm3/stm32/rcc.h>
#include "Units.h"


namespace time {
    struct Config {
        uint32_t timer;
        uint32_t timer_clock_freq;
    };

    void setup(Config& config);
    uint64_t getMsTicks();

    class Delay {
    public:
        Delay(Microsecond time);
        void wait();
        bool hasExpired();
    
    private:
        bool m_systick_delay; 
        uint64_t m_start;
        uint32_t m_raw_time;
    };
}