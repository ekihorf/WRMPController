#pragma once

#include <cstdint>
#include <libopencm3/stm32/rcc.h>

class Microsecond {
public:
    constexpr explicit Microsecond(uint32_t us) : value{us} {}

    uint32_t value;
};

constexpr Microsecond operator "" _us(unsigned long long us) {
    return Microsecond(static_cast<uint32_t>(us));
}

constexpr Microsecond operator "" _ms(unsigned long long ms) {
    return Microsecond(static_cast<uint32_t>(ms) * 1000);
}

namespace time {
    struct Config {
        uint32_t timer;
        uint32_t timer_clock_freq;
    };

    void setup(Config& config);
    uint32_t getSystickTicks();

    class Delay {
    public:
        Delay(Microsecond time);
        void wait();
        bool hasExpired();
    
    private:
        bool m_systick_delay; 
        uint32_t m_start;
        uint32_t m_raw_time;
    };
}