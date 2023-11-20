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
#include <libopencm3/stm32/iwdg.h>
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
#include "Button.h"
#include "Ui.h"
#include "DeviceState.h"
#include "Constants.h"

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
}

struct TaskContext {
	Pid& pid;
	AcControl& heater;
	TempSensor& sensor;
	Encoder& encoder;
	Button& button;
	CharDisplay& display;
	DebugOut& debug;
	ui::Ui& ui;
	DeviceState& state;
};

static void pid_task_func(void* data) {
	auto& d = *static_cast<TaskContext*>(data);

	d.heater.turnOff();
	time::Delay(200_us).wait();
	d.sensor.performConversion();
	d.state.tip_temp = d.sensor.getTemperature();
	d.state.heater_power = d.pid.calculate(d.state.tip_temp.value, d.state.set_temp.value);

	if (d.state.tip_temp > MAX_TIP_TEMP) {
		d.heater.turnOff();
		return;
	}

	if (d.state.heating_status == HeatingStatus::On) {
		d.heater.turnOn(d.state.heater_power / 5);
	}
}

char disp_buf[8] = {};
ui::Buffer ui_buf;

static void ui_task_func(void* data) {
	auto& d = *static_cast<TaskContext*>(data);
	iwdg_reset(); // TODO: is this the right place for this?

	// ugly but whatever
	auto delta = d.encoder.getDelta();
	uint32_t n = abs(delta);
	auto event = delta < 0 ? ui::Event::EncoderCCW : ui::Event::EncoderCW;
	while (n--) {
		d.ui.handleEvent(event);
	}

	d.ui.draw(ui_buf);
	d.display.goTo(0, 0);
	d.display.printN(ui_buf.line1, 16);

	d.display.goTo(1, 0);
	d.display.printN(ui_buf.line2, 16);
	// d.display.goTo(0, 0);
	// d.display.print("Set temp: ");
	// utils::uintToStr(disp_buf, d.set_temp, 3);
	// disp_buf[3] = 0xDF;
	// disp_buf[4] = 'C';

	// d.display.print(disp_buf);

	// d.display.goTo(1, 0);
	// d.display.print("Cur temp: ");
	// utils::uintToStr(disp_buf, d.tip_temp, 3);
	// disp_buf[3] = 0xDF;
	// disp_buf[4] = 'C';
	// d.display.print(disp_buf);
	
	d.display.flushBuffer();
}

static void btn_task_func(void* data) {
	auto& d = *static_cast<TaskContext*>(data);

	auto event = d.button.update();

	if (event == Button::EventType::ShortPress) {
		d.ui.handleEvent(ui::Event::ButtonShortPress);
	} else if (event == Button::EventType::LongPress) {
		d.ui.handleEvent(ui::Event::ButtonLongPress);
	}
}

static void debug_task_func(void* data) {
	auto& d = *static_cast<TaskContext*>(data);

	DebugData debug_data {
		.tip_temp = d.state.tip_temp.value,
		.cj_temp = 30,
		.duty_cycle = d.state.tip_temp.value
	};

	d.debug.sendData(debug_data);
}

int main()
{
	rcc_clock_setup(&rcc_clock_config[RCC_CLOCK_CONFIG_HSI_16MHZ]);
	iwdg_set_period_ms(1500);
	iwdg_start();
	
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
	
	time::Delay(50_ms).wait();

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

	Button button(GPIOA, GPIO5, true);


	Celsius set_temp = 200_degC;
	Celsius standby_temp = 150_degC;
	Celsius tip_temp = 0_degC;
	uint32_t pid_output = 0;
	HeatingStatus heating_status = HeatingStatus::Off;
	uint32_t temp_increment = 5;

	DeviceState device_state {
		.set_temp = set_temp,
		.tip_temp = tip_temp,
		.standby_temp = standby_temp,
		.heater_power = pid_output,
		.heating_status = heating_status,
		.temp_increment = temp_increment
	};

	ui::Ui main_ui(device_state);

	TaskContext task_context {
		.pid = pid,
		.heater = heater,
		.sensor = sensor,
		.encoder = encoder,
		.button = button,
		.display = display,
		.debug = debug,
		.ui = main_ui,
		.state = device_state
	};
	
	Task pid_task(5_ms, 200_ms, pid_task_func);
	pid_task.setData(&task_context);
	Task ui_task(5_ms, 100_ms, ui_task_func);
	ui_task.setData(&task_context);
	Task debug_task(5_ms, 500_ms, debug_task_func);
	debug_task.setData(&task_context);
	Task button_task(1_ms, 5_ms, btn_task_func);
	button_task.setData(&task_context);

	Scheduler scheduler;
	scheduler.addTask(ui_task);
	scheduler.addTask(pid_task);
	scheduler.addTask(debug_task);
	scheduler.addTask(button_task);

	time::Delay(50_ms).wait();
	display.print("INIT");
	display.flushBuffer();

	time::Delay(50_ms).wait();

	while (1) {
		scheduler.runNextTask();
	}

	return 0;
}