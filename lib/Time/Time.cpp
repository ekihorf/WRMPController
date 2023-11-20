#include "Time.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

static volatile uint32_t ticks = 0;
static uint32_t timer_peripheral;

void time::setup(uint32_t timer, rcc_periph_clken timer_rcc) {
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(rcc_ahb_frequency / 1000);
	systick_counter_enable();
	systick_interrupt_enable();

	timer_peripheral = timer;
	rcc_periph_clock_enable(timer_rcc);
	timer_set_mode(timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(timer, (rcc_apb1_frequency) / 1'000'000);
    timer_disable_preload(timer);
    timer_continuous_mode(timer);
    timer_set_period(timer, 65535);
	timer_enable_counter(timer);
}

uint32_t time::getSystickTicks() {
    return ticks;
}

time::Delay::Delay(Microsecond delay) {
	if (delay.value > 65535) {
		m_systick_delay = true;
		m_start = ticks;
		m_raw_time = delay.value / 1000;
	} else {
		m_systick_delay = false;
		m_start = timer_get_counter(timer_peripheral);
		m_raw_time = delay.value;
	}
}

void time::Delay::wait() {
    while (!hasExpired());
}

bool time::Delay::hasExpired() {
    if (m_systick_delay) {
		return ticks - m_start >= m_raw_time;
	} else {
		return timer_get_counter(timer_peripheral) - m_start >= m_raw_time;
	}
}

extern "C" void sys_tick_handler(void) {
	++ticks;
}
