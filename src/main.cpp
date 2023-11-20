#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/adc.h>
#include "AcControl.h"
#include "Debug.h"
#include "SysTick.h"
#include "TempSensor.h"

#define PORT_HEATER GPIOA
#define PIN_HEATER GPIO4

#define PORT_ZERO GPIOA
#define PIN_ZERO GPIO0

AcControl heater(TIM14, TIM_OC1, RCC_TIM14);

static void gpio_setup()
{
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_mode_setup(PORT_HEATER, GPIO_MODE_AF, GPIO_PUPD_NONE, PIN_HEATER);
	gpio_set_af(PORT_HEATER, GPIO_AF4, PIN_HEATER);

	gpio_mode_setup(PORT_ZERO, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, PIN_ZERO);
	gpio_clear(PORT_HEATER, PIN_HEATER);

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
	heater.zeroCrossingCallback();
	exti_reset_request(EXTI0);
}

int main()
{
	rcc_clock_setup(&rcc_clock_config[RCC_CLOCK_CONFIG_HSI_16MHZ]);
	gpio_setup();
	usart_setup();
	systick::setup();

	TempSensor sensor(ADC1, 8, 320, 3270);

	DebugOut debug(USART2);

	while (1) {
		heater.turnOff();
		systick::Delay delay(200);
		sensor.performConversion();

		heater.turnOn(1);

		DebugData d {
			.tip_temp = sensor.getTemperature(),
			.cj_temp = 30,
			.duty_cycle = 20
		};

		debug.sendData(d);

		delay.wait();
	}

	return 0;
}