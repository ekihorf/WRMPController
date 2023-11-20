#include "Irqs.h"
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/assert.h>

static InterruptHandler g_exti0_handler;

static InterruptHandler g_tim3_handler;

InterruptHandler& Irqs::getExtiHandler(uint32_t exti) {
    switch (exti) {
        case EXTI0: return g_exti0_handler;
    }

    cm3_assert(false);
}

InterruptHandler& Irqs::getTimerHandler(uint32_t exti) {
    switch (exti) {
        case TIM3: return g_tim3_handler;
    }

    cm3_assert(false);
}

extern "C" void exti0_1_isr(void) {
    g_exti0_handler();
}

extern "C" void tim3_isr(void) {
    g_tim3_handler();
}