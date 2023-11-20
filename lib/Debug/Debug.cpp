#include "Debug.h"
#include <libopencm3/stm32/usart.h>
#include "Utils.h"

DebugOut::DebugOut(uint32_t usart, rcc_periph_clken usart_rcc)
: m_usart{usart} {
	rcc_periph_clock_enable(usart_rcc);
    usart_set_baudrate(usart, 115200);
	usart_set_databits(usart, 8);
	usart_set_parity(usart, USART_PARITY_NONE);
	usart_set_stopbits(usart, USART_CR2_STOPBITS_1);
	usart_set_mode(usart, USART_MODE_TX);
	usart_set_flow_control(usart, USART_FLOWCONTROL_NONE);

	usart_enable(usart);
};

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