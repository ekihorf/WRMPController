#pragma once

#include <cstdint>

namespace systick {
    void setup();
    uint32_t getTicks();

    class Delay {
    public:
        Delay(uint32_t time_ms);
        void wait();
        bool hasExpired();
    
    private:
        uint32_t m_end;
    };
}