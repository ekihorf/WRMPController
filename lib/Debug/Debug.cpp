#include "Debug.h"
#include <libopencm3/stm32/usart.h>
#include "Utils.h"

DebugOut::DebugOut(uint32_t usart) : m_usart{usart} {};

void DebugOut::sendData(DebugData& data) {
    char buf[16];
    utils::uintToStr(buf, data.tip_temp, 3);
    buf[3] = ' ';
    utils::uintToStr(buf + 4, data.cj_temp, 3);
    buf[7] = ' ';
    utils::uintToStr(buf + 8, data.duty_cycle, 3);
    buf[11] = '\r';
    buf[12] = '\n';
    buf[13] = '\0';

    char* ptr = buf;
    while(*ptr) {
        usart_send_blocking(m_usart, *(ptr++));
    }
}