#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/i2c.h>
#include "AcControl.h"
#include "Debug.h"
#include "Time.h"
#include "TempSensor.h"
#include "Pid.h"
#include "CharDisplay.h"
#include "Utils.h"
#include "InterruptHandler.h"

constexpr uint32_t PORT_HEATER{GPIOA};
constexpr uint16_t PIN_HEATER{GPIO4};

constexpr uint32_t PORT_ZERO{GPIOA};
constexpr uint16_t PIN_ZERO{GPIO0};

static void gpio_setup()
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_TIM14);
	rcc_periph_clock_enable(RCC_TIM16);
	nvic_enable_irq(NVIC_EXTI0_1_IRQ);

	// UART TX pin. Used by debug module
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);

	// I2C pins
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF6, GPIO11 | GPIO12);

	// Encoder pins
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO6 | GPIO7);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO6 | GPIO7);
}

static void i2c_setup() {
	rcc_periph_clock_enable(RCC_I2C2);
	i2c_peripheral_disable(I2C2);
	i2c_enable_analog_filter(I2C2);
	i2c_set_digital_filter(I2C2, 0);
	i2c_set_speed(I2C2, i2c_speed_sm_100k, rcc_apb1_frequency / 1'000'000);
	i2c_set_7bit_addr_mode(I2C2);
	i2c_peripheral_enable(I2C2);
}

int main()
{
	rcc_clock_setup(&rcc_clock_config[RCC_CLOCK_CONFIG_HSI_16MHZ]);
	
	gpio_setup();

	time::Config timeConfig {
		.timer = TIM16,
		.timer_clock_freq = rcc_apb1_frequency
	};
	time::setup(timeConfig);

	i2c_setup();

	AcControl::Config heaterConfig {
		.timer = TIM14,
		.timer_oc = TIM_OC1,
		.timer_clock_freq = rcc_apb1_frequency,
	    .zero_cross_port = GPIOA,
		.zero_cross_pin = GPIO0,
		.zero_cross_exti = EXTI0,
		.heater_port = GPIOA,
		.heater_pin = GPIO4,
		.heater_pin_af = GPIO_AF4 
	};

	AcControl heater(heaterConfig);
	
	time::Delay(200_ms).wait();

	CharDisplay display(I2C2, 0x27);
	// Encoder encoder(TIM3, RCC_TIM3);
		
	TempSensor sensor(ADC1, 8, 320, 3270);
	Pid pid(200);
	pid.setTunings(1100, 100, 500);
	pid.setLimits(0, 90);

	DebugOut debug(USART2, RCC_USART2);

	char buf[8] = {};

	int32_t counter = 0; 

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
		
		// counter += encoder.getDelta();

		
		display.goTo(0, 0);
		display.print("Counter: ");
		utils::uintToStr(buf, counter, 4);
		display.print(buf);

		display.goTo(1, 0);
		display.print("Actual temp: ");
		utils::uintToStr(buf, tip_temp, 3);
		display.print(buf);

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