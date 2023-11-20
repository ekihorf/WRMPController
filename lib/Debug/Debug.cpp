#include "Debug.h"
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include "Utils.h"

Debug::Debug(const Config& config)
: m_usart{config.usart} {
    gpio_mode_setup(config.gpio_port, GPIO_MODE_AF, GPIO_PUPD_NONE, config.gpio_pins);
	gpio_set_af(config.gpio_port, config.gpio_af, config.gpio_pins);

    usart_set_baudrate(m_usart, 115200);
	usart_set_databits(m_usart, 8);
	usart_set_parity(m_usart, USART_PARITY_NONE);
	usart_set_stopbits(m_usart, USART_CR2_STOPBITS_1);
	usart_set_mode(m_usart, USART_MODE_TX);
	usart_set_flow_control(m_usart, USART_FLOWCONTROL_NONE);

	usart_enable(m_usart);
};

void Debug::sendData(Data& data) {
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