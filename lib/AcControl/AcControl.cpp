#include "AcControl.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>

#include "InterruptHandler.h"
#include "Irqs.h"

AcControl::AcControl(Config& config)
: m_timer{config.timer},
  m_timer_oc{config.timer_oc},
  m_exti{config.zero_cross_exti} {
    Irqs::getExtiHandler(m_exti).connect<&AcControl::extiIsr>(this);
    
	gpio_mode_setup(config.heater_port, GPIO_MODE_AF, GPIO_PUPD_NONE, config.heater_pin);
	gpio_set_af(config.heater_port, config.heater_pin_af, config.heater_pin);

	gpio_mode_setup(config.zero_cross_port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, config.zero_cross_pin);
	exti_select_source(m_exti, config.zero_cross_port);
	exti_set_trigger(m_exti, EXTI_TRIGGER_BOTH);
	exti_enable_request(m_exti);

    timer_set_mode(m_timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(m_timer, config.timer_clock_freq / 100000);
    timer_disable_preload(m_timer);
    timer_one_shot_mode(m_timer);
    timer_set_period(m_timer, 100);
    timer_set_oc_value(m_timer, m_timer_oc, 1);
    timer_set_oc_mode(m_timer, m_timer_oc, TIM_OCM_INACTIVE);
    timer_enable_oc_output(m_timer, m_timer_oc);
}

void AcControl::turnOn(uint32_t on_halfcycles) {
    m_remaining_halfcycles = on_halfcycles;
    timer_set_oc_mode(m_timer, m_timer_oc, TIM_OCM_PWM2);
}

void AcControl::turnOff() {
    m_remaining_halfcycles = 0;
    timer_set_oc_mode(m_timer, m_timer_oc, TIM_OCM_INACTIVE);
}

void AcControl::extiIsr() {
    if (m_remaining_halfcycles > 0) {
        --m_remaining_halfcycles;
        timer_enable_counter(m_timer);
    }
    exti_reset_request(m_exti);
}