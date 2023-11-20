#include "Time.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/cortex.h>
#include <etl/atomic.h>

// Never read this variable without disabling interrupts
static uint64_t g_ms_ticks = 0;
static uint32_t g_timer;

void time::setup(time::Config& config) {
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(rcc_ahb_frequency / 1000);
	systick_counter_enable();
	systick_interrupt_enable();

	g_timer = config.timer;
	timer_set_mode(g_timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(g_timer, config.timer_clock_freq / 1'000'000);
    timer_disable_preload(g_timer);
    timer_continuous_mode(g_timer);
    timer_set_period(g_timer, 65535);
	timer_enable_counter(g_timer);
}

uint64_t time::getMsTicks() {
	cm_disable_interrupts();
	uint64_t ticks = g_ms_ticks;
	cm_enable_interrupts();
    return ticks;
}

time::Delay::Delay(Duration delay) {
	if (delay.asMicroseconds() > 65535) {
		m_systick_delay = true;
		m_start = getMsTicks();
		m_raw_time = delay.asMilliseconds();
	} else {
		m_systick_delay = false;
		m_start = timer_get_counter(g_timer);
		m_raw_time = delay.asMicroseconds();
	}
}

void time::Delay::wait() {
    while (!hasExpired());
}

bool time::Delay::hasExpired() {
    if (m_systick_delay) {
		return getMsTicks() - m_start >= m_raw_time;
	} else {
		uint16_t diff = static_cast<uint16_t>(timer_get_counter(g_timer)) - static_cast<uint16_t>(m_start);
		return diff >= static_cast<uint16_t>(m_raw_time);
	}
}

extern "C" void sys_tick_handler(void) {
	++g_ms_ticks;
}