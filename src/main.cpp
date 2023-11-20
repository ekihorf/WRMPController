#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/dma.h>
#include "AcControl.h"
#include "Debug.h"
#include "Time.h"
#include "Scheduler.h"
#include "TempSensor.h"
#include "Pid.h"
#include "CharDisplay.h"
#include "Utils.h"
#include "InterruptHandler.h"
#include "Encoder.h"
#include "I2cDma.h"

constexpr uint32_t PORT_HEATER{GPIOA};
constexpr uint16_t PIN_HEATER{GPIO4};

constexpr uint32_t PORT_ZERO{GPIOA};
constexpr uint16_t PIN_ZERO{GPIO0};

static void gpio_setup()
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_TIM14);
	rcc_periph_clock_enable(RCC_TIM16);
	rcc_periph_clock_enable(RCC_TIM3);
	nvic_enable_irq(NVIC_EXTI0_1_IRQ);

	// UART TX pin. Used by debug module
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);

	// I2C pins
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF6, GPIO11 | GPIO12);
}

static void i2c_setup() {
	rcc_periph_clock_enable(RCC_I2C2);
	rcc_periph_clock_enable(RCC_DMA1);
	nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);
	// i2c_peripheral_disable(I2C2);
	// i2c_enable_analog_filter(I2C2);
	// i2c_set_digital_filter(I2C2, 0);
	// i2c_set_speed(I2C2, i2c_speed_sm_100k, rcc_apb1_frequency / 1'000'000);
	// i2c_set_7bit_addr_mode(I2C2);
	// i2c_peripheral_enable(I2C2);
}

struct TaskContext {
	Pid& pid;
	AcControl& heater;
	TempSensor& sensor;
	Encoder& encoder;
	CharDisplay& display;
	DebugOut& debug;
	int32_t& set_temp;
	uint32_t& tip_temp;
	uint32_t& pid_output;
};

static void pid_task_func(void* data) {
	auto& d = *static_cast<TaskContext*>(data);

	d.heater.turnOff();
	time::Delay(200_us).wait();
	d.sensor.performConversion();
	d.tip_temp = d.sensor.getTemperature().value;
	d.pid_output = d.pid.calculate(d.tip_temp, d.set_temp);
	if (d.tip_temp < 500) {
		d.heater.turnOn(d.pid_output / 5);
	}
}

char disp_buf[8] = {};

static void ui_task_func(void* data) {
	auto& d = *static_cast<TaskContext*>(data);

	d.set_temp += d.encoder.getDelta() * 5;
	if (d.set_temp > 450) {
		d.set_temp = 450;
	} else if (d.set_temp < 100) {
		d.set_temp = 100;
	}

	d.display.goTo(0, 0);
	d.display.print("Set temp: ");
	utils::uintToStr(disp_buf, d.set_temp, 3);
	d.display.print(disp_buf);

	d.display.goTo(1, 0);
	d.display.print("Actual temp: ");
	utils::uintToStr(disp_buf, d.tip_temp, 3);
	d.display.print(disp_buf);

	d.display.flushBuffer();
}

static void debug_task_func(void* data) {
	auto& d = *static_cast<TaskContext*>(data);

	DebugData debug_data {
		.tip_temp = d.tip_temp,
		.cj_temp = 30,
		.duty_cycle = d.pid_output
	};

	d.debug.sendData(debug_data);
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

	I2cDma::Config i2c_config {
		.i2c = I2C2,
		.apb1_freq = rcc_apb1_frequency,
		.dma = DMA1,
		.dma_channel = DMA_CHANNEL1
	};
	I2cDma i2c(i2c_config);

	AcControl::Config heater_config {
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

	AcControl heater(heater_config);
	
	time::Delay(200_ms).wait();

	CharDisplay::Config disp_config {
		.i2cdma = i2c,
		.i2c_addr = 0x27
	};
	CharDisplay display(disp_config);
	display.flushBuffer();

	Encoder::Config encoder_config {
		.timer = TIM3,
		.port_a = GPIOA,
		.pin_a = GPIO6,
		.port_b = GPIOA,
		.pin_b = GPIO7,
		.gpio_af = GPIO_AF1
	};
	Encoder encoder(encoder_config);

	TempSensor::Config temp_config {
		.adc = ADC1,
		.channel = 8,
		.amp_gain = 315,
		.vref = 3270_mV
	};
	TempSensor sensor(temp_config);
	sensor.setOffset(15_degC);

	Pid pid(200);
	pid.setTunings(1100, 100, 500);
	pid.setLimits(0, 90);

	DebugOut debug(USART2, RCC_USART2);

	int32_t set_temp = 0; 
	uint32_t tip_temp = 0;
	uint32_t pid_output = 0;

	TaskContext task_context {
		.pid = pid,
		.heater = heater,
		.sensor = sensor,
		.encoder = encoder,
		.display = display,
		.debug = debug,
		.set_temp = set_temp,
		.tip_temp = tip_temp,
		.pid_output = pid_output
	};
	
	Task pid_task(5_ms, 200_ms, pid_task_func);
	pid_task.setData(&task_context);
	Task ui_task(5_ms, 100_ms, ui_task_func);
	ui_task.setData(&task_context);
	Task debug_task(5_ms, 500_ms, debug_task_func);
	debug_task.setData(&task_context);

	Scheduler scheduler;
	scheduler.addTask(ui_task);
	scheduler.addTask(pid_task);
	scheduler.addTask(debug_task);

	time::Delay(100_ms).wait();
	display.print("INIT");
	display.flushBuffer();

	time::Delay(500_ms).wait();

	while (1) {
		scheduler.runNextTask();
	}

	return 0;
}