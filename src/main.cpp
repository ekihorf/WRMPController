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
#include "Nvs.h"
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
	rcc_periph_clock_enable(RCC_CRC);
	nvic_enable_irq(NVIC_EXTI0_1_IRQ);

	// UART TX pin. Used by debug module
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);

	// I2C pins
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF6, GPIO11 | GPIO12);
}

static void i2c_setup() {
	rcc_periph_clock_enable(RCC_I2C2);
	rcc_periph_clock_enable(RCC_DMA1);
	nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);
}

struct Context {
	Pid& pid;
	AcControl& heater;
	TempSensor& sensor;
	Encoder& encoder;
	Button& button;
	CharDisplay& display;
	DebugOut& debug;
	ui::Ui& ui;
	DeviceState& state;
	Nvs& nvs;
};

static void pid_task_func(void* data) {
	auto& d = *static_cast<Context*>(data);

	d.heater.turnOff();
	time::Delay(200_us).wait();
	d.sensor.performConversion();
	d.state.tip_temp = d.sensor.getTemperature();
	d.state.heater_power = d.pid.calculate(d.state.tip_temp.asDegreesC(), d.state.set_temp.asDegreesC());

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
	auto& d = *static_cast<Context*>(data);
	iwdg_reset(); // TODO: is this the right place for this?

	auto delta = d.encoder.getDelta();
	uint32_t n = abs(delta);
	auto event = delta < 0 ? ui::Event::EncoderCCW : ui::Event::EncoderCW;
	while (n--) {
		d.ui.handleEvent(event);
	}

	if (d.state.temp_updated) {
		if (d.nvs.writeSts(d.state.set_temp.asDegreesC())) {
			d.state.temp_updated = false;
		}
	} else if (d.state.settings_updated) {
		d.state.settings_updated = false;
		d.nvs.writeLts(&d.state.settings);
		d.pid.setTunings(d.state.settings.pid_kp, d.state.settings.pid_ki, d.state.settings.pid_kd);
		d.sensor.setVref(d.state.settings.tc_vref);
		d.sensor.setAmpGain(d.state.settings.tc_amp_gain);
		d.sensor.setOffset(d.state.settings.tc_offset);
	}

	d.ui.draw(ui_buf);

	d.display.goTo(0, 0);
	d.display.printN(ui_buf.line1, 16);
	d.display.goTo(1, 0);
	d.display.printN(ui_buf.line2, 16);
	
	d.display.flushBuffer();
}

static void btn_task_func(void* data) {
	auto& d = *static_cast<Context*>(data);

	auto event = d.button.update();

	if (event == Button::EventType::ShortPress) {
		d.ui.handleEvent(ui::Event::ButtonShortPress);
	} else if (event == Button::EventType::LongPress) {
		d.ui.handleEvent(ui::Event::ButtonLongPress);
	}
}

static void debug_task_func(void* data) {
	auto& d = *static_cast<Context*>(data);

	DebugData debug_data {
		.tip_temp = static_cast<uint32_t>(d.state.tip_temp.asDegreesC()),
		.cj_temp = 30,
		.duty_cycle = static_cast<uint32_t>(d.state.tip_temp.asDegreesC())
	};

	d.debug.sendData(debug_data);
}

static void msg(Context& ctx, const char* text) {
	ctx.display.goTo(0, 0);
	ctx.display.printNBlocking(text, 16);
}


static void load_settings(Context& ctx) {
	bool result = ctx.nvs.readLts(&ctx.state.settings);
	if (result && !ctx.button.isPressedRaw()) {
		auto sts = ctx.nvs.readSts();
		if (sts.has_value()) {
			auto temp = Temperature(sts.value());
			if (temp <= MAX_TIP_TEMP && temp >= MIN_TIP_TEMP) {
				ctx.state.set_temp = temp;
			}
		}
		return;
	}

	msg(ctx, "EEPROM INIT...");
	time::Delay(200_ms).wait();

	result = ctx.nvs.erase();
	if (!result) {
		msg(ctx, "EEPROM ERROR 01");
		while (true)
			;
	}

	ctx.state.settings = defaults::SETTINGS;
	result = ctx.nvs.writeLts(&ctx.state.settings);
	if (!result) {
		msg(ctx, "EEPROM ERROR 02");
		while (true)
			;
	}

	msg(ctx, "EEPROM INIT OK");
	time::Delay(1_s).wait();
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
		.amp_gain = 340,
		.vref = 3295_mV
	};
	TempSensor sensor(temp_config);

	Pid pid(200);
	pid.setLimits(0, 90);

	Nvs::Config nvs_config {
		.i2c = i2c,
		.i2c_addr = 0x50,
		.eeprom_size = 1024,
		.lts_size = sizeof(DeviceSettings),
		.sts_start = 64,
	};

	Nvs nvs(nvs_config);
	DebugOut debug(USART2, RCC_USART2);
	Button button(GPIOA, GPIO5, true);

	DeviceState device_state {
		.set_temp = 200_degC,
		.tip_temp = 0_degC,
		.heater_power = 0,
		.heating_status = HeatingStatus::Off,
		.temp_updated = false,
		.settings_updated = false,
		.settings = defaults::SETTINGS
	};

	ui::Ui main_ui(device_state);

	ui::Parameter param_temp_increment("Temp. increment", "\337C", device_state.settings.temp_increment.value, 5, 20, 5);
	ui::Parameter param_standby_temp("Standby temp.", "\337C", device_state.settings.standby_temp.value, 100, 250, 5);
	ui::Parameter param_standby_delay("Standby delay", "S", reinterpret_cast<int32_t&>(device_state.settings.standby_delay.value), 0, 600'000'000, 10'000'000, 6, false);
	ui::Parameter param_off_delay("Off delay", "S", reinterpret_cast<int32_t&>(device_state.settings.off_delay.value), 0, 600'000'000, 10'000'000, 6, false);
	ui::Parameter param_tc_vref("TC Vref", "mV", reinterpret_cast<int32_t&>(device_state.settings.tc_vref.value), 3200, 3400, 1);
	ui::Parameter param_tc_offset("TC offset", "\337C", device_state.settings.tc_offset.value, -100, 100, 1);
	ui::Parameter param_tc_amp_gain("TC amp gain", "", device_state.settings.tc_amp_gain, 250, 400, 1);
	ui::Parameter param_pid_kp("PID Kp", "", device_state.settings.pid_kp, 100, 1800, 10, 3);
	ui::Parameter param_pid_ki("PID Ki", "", device_state.settings.pid_ki, 100, 1800, 10, 3);
	ui::Parameter param_pid_kd("PID Kd", "", device_state.settings.pid_kd, 100, 1800, 10, 3);
	main_ui.addParameter(param_temp_increment);
	main_ui.addParameter(param_standby_temp);
	main_ui.addParameter(param_standby_delay);
	main_ui.addParameter(param_off_delay);
	main_ui.addParameter(param_tc_vref);
	main_ui.addParameter(param_tc_offset);
	main_ui.addParameter(param_tc_amp_gain);
	main_ui.addParameter(param_pid_kp);
	main_ui.addParameter(param_pid_ki);
	main_ui.addParameter(param_pid_kd);

	Context task_context {
		.pid = pid,
		.heater = heater,
		.sensor = sensor,
		.encoder = encoder,
		.button = button,
		.display = display,
		.debug = debug,
		.ui = main_ui,
		.state = device_state,
		.nvs = nvs
	};


	time::Delay(50_ms).wait();
	load_settings(task_context);

	pid.setTunings(device_state.settings.pid_kp, device_state.settings.pid_ki, device_state.settings.pid_kd);
	sensor.setAmpGain(device_state.settings.tc_amp_gain);
	sensor.setOffset(device_state.settings.tc_offset);
	sensor.setVref(device_state.settings.tc_vref);


	iwdg_set_period_ms(1500);
	// iwdg_start();
	
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