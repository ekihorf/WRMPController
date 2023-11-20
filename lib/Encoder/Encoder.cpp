#include "Encoder.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

Encoder::Encoder(Config& config)
: m_timer{config.timer},
  m_port_a{config.port_a}, 
  m_pin_a{config.pin_a},
  m_port_b{config.port_b},
  m_pin_b{config.pin_b} {
    gpio_mode_setup(m_port_a, GPIO_MODE_AF, GPIO_PUPD_PULLUP, m_pin_a);
    gpio_mode_setup(m_port_b, GPIO_MODE_AF, GPIO_PUPD_PULLUP, m_pin_b);
    gpio_set_af(m_port_a, config.gpio_af, m_pin_a);
    gpio_set_af(m_port_b, config.gpio_af, m_pin_b);

    timer_set_mode(m_timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_enable_preload(m_timer);
    timer_set_period(m_timer, 65535);
    timer_slave_set_mode(m_timer, TIM_SMCR_SMS_EM3);
    timer_slave_set_polarity(m_timer, TIM_ET_RISING);
    timer_slave_set_filter(m_timer, TIM_IC_DTF_DIV_32_N_8);
    timer_enable_counter(m_timer);
}

int32_t Encoder::getDelta() {
    uint16_t cur = timer_get_counter(m_timer);
    int32_t raw_delta = cur - m_last;
    int8_t delta = raw_delta / 4;
    m_last = cur - raw_delta % 4;
    return delta;
}