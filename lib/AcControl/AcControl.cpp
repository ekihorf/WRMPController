#include "AcControl.h"
#include <libopencm3/stm32/gpio.h>

AcControl::AcControl(uint32_t timer, tim_oc_id timer_oc, rcc_periph_clken timer_rcc)
: m_timer{timer},
  m_timer_oc{timer_oc} {
	rcc_periph_clock_enable(timer_rcc);
    timer_set_mode(timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(timer, (rcc_apb1_frequency) / 100000);
    timer_disable_preload(timer);
    timer_one_shot_mode(timer);
    timer_set_period(timer, 100);
    timer_set_oc_value(timer, timer_oc, 1);
    timer_set_oc_mode(timer, timer_oc, TIM_OCM_INACTIVE);
    timer_enable_oc_output(timer, timer_oc);
}

void AcControl::turnOn(uint32_t on_halfcycles) {
    m_remaining_halfcycles = on_halfcycles;
    timer_set_oc_mode(m_timer, m_timer_oc, TIM_OCM_PWM2);
}

void AcControl::turnOff() {
    m_remaining_halfcycles = 0;
    timer_set_oc_mode(m_timer, m_timer_oc, TIM_OCM_INACTIVE);
}

void AcControl::zeroCrossingCallback() {
    if (m_remaining_halfcycles > 0) {
        --m_remaining_halfcycles;
        timer_enable_counter(m_timer);
    }
}