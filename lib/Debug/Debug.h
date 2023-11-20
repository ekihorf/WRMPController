#pragma once

#include <cstdint>
#include <libopencm3/stm32/rcc.h>

struct DebugData {
    uint32_t tip_temp;
    uint32_t cj_temp;
    uint32_t duty_cycle;
};

class DebugOut {
public:
    DebugOut(uint32_t usart, rcc_periph_clken usart_rcc);
    void sendData(DebugData& data);

private:
    uint32_t m_usart;
};