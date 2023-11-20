#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "AcControl.h"
#include "Debug.h"

#define PORT_LED GPIOA
#define PIN_LED GPIO1

#define PORT_ZERO GPIOA
#define PIN_ZERO GPIO0

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

	nvic_enable_irq(NVIC_EXTI0_1_IRQ);
	exti_select_source(EXTI0, GPIOA);
	exti_set_trigger(EXTI0, EXTI_TRIGGER_BOTH);
	exti_enable_request(EXTI0);
}

static void usart_setup(void)
{
	rcc_periph_clock_enable(RCC_USART2);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);
	/* Setup USART parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_stopbits(USART2, USART_CR2_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

extern "C" void exti0_1_isr(void) {
	heater.signalZeroCrossing();
	exti_reset_request(EXTI0);
}

extern "C" void sys_tick_handler(void) {
	++ticks;
}

void _putchar(char ch) {
	usart_send_blocking(USART2, ch);
}

int main()
{
	rcc_clock_setup(&rcc_clock_config[RCC_CLOCK_CONFIG_HSI_16MHZ]);
	gpio_setup();
	usart_setup();
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(rcc_ahb_frequency / 1000);
	systick_counter_enable();
	systick_interrupt_enable();

	heater.setOnHalfcycles(5);

	uint32_t error_wait = 0;
	bool error_found = false;
	heater.setControlPeriod(20);

	DebugOut debug(USART2);

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

		DebugData d {
			.tip_temp = 314,
			.cj_temp = 30,
			.duty_cycle = 20
		};

		debug.sendData(d);

		while (wake > ticks);
	}

	return 0;
}