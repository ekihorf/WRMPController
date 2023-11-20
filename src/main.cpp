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
#include "Time.h"
#include "TempSensor.h"
#include "Pid.h"

constexpr uint32_t PORT_HEATER{GPIOA};
constexpr uint16_t PIN_HEATER{GPIO4};

constexpr uint32_t PORT_ZERO{GPIOA};
constexpr uint16_t PIN_ZERO{GPIO0};

// FIXME: this is ugly, but needed now because heater object is used in an ISR.
AcControl heater(TIM14, TIM_OC1, RCC_TIM14);

static void gpio_setup()
{
	rcc_periph_clock_enable(RCC_GPIOA);

	// Heater pin. Used by TIM14 CH1
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, PIN_HEATER);
	gpio_set_af(GPIOA, GPIO_AF4, PIN_HEATER);

	// Zero crossing detection input pin
	gpio_mode_setup(PORT_ZERO, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, PIN_ZERO);
	// Zero crossing detection interrupt
	nvic_enable_irq(NVIC_EXTI0_1_IRQ);
	exti_select_source(EXTI0, PORT_ZERO);
	exti_set_trigger(EXTI0, EXTI_TRIGGER_BOTH);
	exti_enable_request(EXTI0);

	// UART TX pin. Used by debug module
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);
}

extern "C" void exti0_1_isr(void) {
	heater.zeroCrossingCallback();
	exti_reset_request(EXTI0);
}

int main()
{
	rcc_clock_setup(&rcc_clock_config[RCC_CLOCK_CONFIG_HSI_16MHZ]);
	gpio_setup();
	time::setup(TIM16, RCC_TIM16);

	TempSensor sensor(ADC1, 8, 320, 3270);
	Pid pid(200);
	pid.setTunings(1100, 100, 500);
	pid.setLimits(0, 90);

	DebugOut debug(USART2, RCC_USART2);

	while (1) {
		heater.turnOff();
		time::Delay delay(200_ms);
		time::Delay(1_ms).wait();
		sensor.performConversion();
		uint32_t tip_temp = sensor.getTemperature();
		uint32_t output = pid.calculate(tip_temp, 300);
		if (tip_temp < 500) {
			heater.turnOn(output / 5);
		}


		DebugData d {
			.tip_temp = tip_temp,
			.cj_temp = 30,
			.duty_cycle = output
		};

		debug.sendData(d);

		delay.wait();
	}

	return 0;
}