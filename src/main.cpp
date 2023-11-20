#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "AcControl.h"

#define PORT_LED GPIOA
#define PIN_LED GPIO2

#define PORT_ZERO GPIOA
#define PIN_ZERO GPIO3

AcControl heater(PORT_ZERO, PIN_ZERO, PORT_LED, PIN_LED);

volatile uint32_t ticks = 0;

static void gpio_setup()
{
	/* Enable GPIOC clock. */
	/* Manually: */
	//RCC_AHBENR |= RCC_AHBENR_GPIOCEN;
	/* Using API functions: */
	rcc_periph_clock_enable(RCC_GPIOA);


	/* Set GPIO8 (in GPIO port C) to 'output push-pull'. */
	/* Using API functions: */
	gpio_mode_setup(PORT_LED, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PIN_LED);
	gpio_mode_setup(PORT_ZERO, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, PIN_ZERO);
	gpio_clear(PORT_LED, PIN_LED);

	nvic_enable_irq(NVIC_EXTI2_3_IRQ);
	exti_select_source(EXTI3, GPIOA);
	exti_set_trigger(EXTI3, EXTI_TRIGGER_BOTH);
	exti_enable_request(EXTI3);
}

extern "C" void exti2_3_isr(void) {
	heater.signalZeroCrossing();
	exti_reset_request(EXTI3);
}

extern "C" void sys_tick_handler(void) {
	++ticks;
}

int main()
{
	gpio_setup();
	rcc_clock_setup(&rcc_clock_config[RCC_CLOCK_CONFIG_HSI_16MHZ]);
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(rcc_ahb_frequency / 1000);
	systick_counter_enable();
	systick_interrupt_enable();

	heater.setOnHalfcycles(5);

	uint32_t error_wait = 0;
	bool error_found = false;

	while (1) {
		uint32_t wake = ticks + 100;
		if (error_wait > 0) {
			error_wait--;
		} else {
			error_found = false;
			heater.setOnHalfcycles(5);
		}

		heater.update(ticks);

		if (!error_found && heater.getStatus() != AcControl::Status::Running) {
			error_wait = 30;
			error_found = true;
		}

		while (wake > ticks);
	}

	return 0;
}