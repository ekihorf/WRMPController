#include "SysTick.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

static volatile uint32_t ticks = 0;

void systick::setup() {
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(rcc_ahb_frequency / 1000);
	systick_counter_enable();
	systick_interrupt_enable();
}

uint32_t systick::getTicks() {
    return ticks;
}

systick::Delay::Delay(uint32_t time_ms) {
    m_end = ticks + time_ms;
}

void systick::Delay::wait() {
    while (!hasExpired());
}

bool systick::Delay::hasExpired() {
    return m_end <= ticks;
}

extern "C" void sys_tick_handler(void) {
	++ticks;
}