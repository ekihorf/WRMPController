#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/iwdg.h>
#include "BoardConfig.h"
#include "Scheduler.h"
#include "Pid.h"
#include "Utils.h"
#include "InterruptHandler.h"
#include "Ui.h"
#include "DeviceState.h"
#include "Constants.h"

struct Context {
	Pid& pid;
	AcControl& heater;
	TempSensor& temp_sensor;
	Encoder& encoder;
	Button& button;
	CharDisplay& display;
	Debug& debug;
	ui::Ui& ui;
	DeviceState& state;
	Nvs& nvs;
	StandbySensor& standby_sensor;
};

static void enablePeripherals()
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_TIM14);
	rcc_periph_clock_enable(RCC_TIM16);
	rcc_periph_clock_enable(RCC_TIM3);
	rcc_periph_clock_enable(RCC_CRC);
	nvic_enable_irq(NVIC_EXTI0_1_IRQ);

	rcc_periph_clock_enable(RCC_I2C2);
	rcc_periph_clock_enable(RCC_DMA1);
	nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);

	rcc_periph_clock_enable(RCC_USART2);
}

static void controlTaskFunc(void* data) {
	auto& d = *static_cast<Context*>(data);

	d.heater.turnOff();
	iwdg_reset();

	switch (d.standby_sensor.update()) {
	case StandbySensor::State::Active:
		d.state.heating_status = HeatingStatus::On;
		break;
	case StandbySensor::State::Standby:
		d.state.heating_status = HeatingStatus::Standby;
		break;
	case StandbySensor::State::Off:
		d.state.heating_status = HeatingStatus::Off;
		break;
	}

	time::Delay(200_us).wait();
	d.temp_sensor.performConversion();
	d.state.tip_temp = d.temp_sensor.getTemperature();
	Temperature temp = d.state.settings.standby_temp;
	if (d.state.heating_status == HeatingStatus::On) {
		temp = d.state.set_temp;
	}
	d.state.heater_power = d.pid.calculate(d.state.tip_temp.asDegreesC(), temp.asDegreesC());

	if (d.state.tip_temp > MAX_TIP_TEMP) {
		d.heater.turnOff();
		return;
	}

	if (d.state.heating_status != HeatingStatus::Off) {
		d.heater.turnOn(d.state.heater_power / 5);
	}
}

static ui::Buffer ui_buf;

static void uiTaskFunc(void* data) {
	auto& d = *static_cast<Context*>(data);

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
		d.temp_sensor.setVref(d.state.settings.tc_vref);
		d.temp_sensor.setAmpGain(d.state.settings.tc_amp_gain);
		d.temp_sensor.setOffset(d.state.settings.tc_offset);
		d.standby_sensor.setDelays(d.state.settings.standby_delay, d.state.settings.off_delay);
	}
	
	d.ui.draw(ui_buf);

	d.display.goTo(0, 0);
	d.display.printN(ui_buf.line1, 16);
	d.display.goTo(1, 0);
	d.display.printN(ui_buf.line2, 16);
	
	d.display.flushBuffer();
}

static void btnTaskFunc(void* data) {
	auto& d = *static_cast<Context*>(data);

	auto event = d.button.update();

	if (event == Button::EventType::ShortPress) {
		if (d.ui.isAtMainView()) {
			d.standby_sensor.reset();
		}
		d.ui.handleEvent(ui::Event::ButtonShortPress);
	} else if (event == Button::EventType::LongPress) {
		d.ui.handleEvent(ui::Event::ButtonLongPress);
	}
}

static void debugTaskFunc(void* data) {
	auto& d = *static_cast<Context*>(data);

	Debug::Data debug_data {
		.tip_temp = static_cast<uint32_t>(d.state.tip_temp.asDegreesC()),
		.cj_temp = 30,
		.duty_cycle = static_cast<uint32_t>(d.state.tip_temp.asDegreesC())
	};

	d.debug.sendData(debug_data);
}

static void printMsg(Context& ctx, const char* text) {
	ctx.display.goTo(0, 0);
	ctx.display.printNBlocking(text, 16);
}

static void loadSettings(Context& ctx) {
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

	printMsg(ctx, "EEPROM INIT...");
	time::Delay(200_ms).wait();

	result = ctx.nvs.erase();
	if (!result) {
		printMsg(ctx, "EEPROM ERROR 01");
		while (true)
			;
	}

	ctx.state.settings = defaults::SETTINGS;
	result = ctx.nvs.writeLts(&ctx.state.settings);
	if (!result) {
		printMsg(ctx, "EEPROM ERROR 02");
		while (true)
			;
	}

	printMsg(ctx, "EEPROM INIT OK");
	time::Delay(1_s).wait();
}

void registerDisplaySymbols(CharDisplay& display) {
	int count = sizeof(ui::SYMBOLS_DATA) / sizeof(ui::SYMBOLS_DATA[0]);
	for (int i = 0; i < count; ++i) {
		display.defineCharacter(i+1, ui::SYMBOLS_DATA[i]);
		display.flushBuffer();
		while(display.isBusBusy())
			;
	}
}

int main()
{
	rcc_clock_setup(&rcc_clock_config[RCC_CLOCK_CONFIG_HSI_16MHZ]);
	
	enablePeripherals();

	time::setup(time_config);

	I2cDma i2c(i2c_config);	
	// Wait for the display to initialize before sending commands
	time::Delay(50_ms).wait();
	CharDisplay display(i2c, disp_config);
	time::Delay(50_ms).wait();
	registerDisplaySymbols(display);

	AcControl heater(heater_config);
	Encoder encoder(encoder_config);
	TempSensor temp_sensor(temp_config);
	StandbySensor standby_sensor(standby_config);
	Nvs nvs(i2c, nvs_config);
	Debug debug(debug_config);
	Button button(button_config);

	Pid pid(CONTROL_PERIOD);
	pid.setLimits(0, MAX_HEATER_POWER);

	DeviceState device_state {
		// This will be overriden with the data from EEPROM later
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
		.temp_sensor = temp_sensor,
		.encoder = encoder,
		.button = button,
		.display = display,
		.debug = debug,
		.ui = main_ui,
		.state = device_state,
		.nvs = nvs,
		.standby_sensor = standby_sensor
	};

	// Make sure that the I2C transfer to the display finished
	time::Delay(50_ms).wait();
	loadSettings(task_context);

	pid.setTunings(device_state.settings.pid_kp, device_state.settings.pid_ki, device_state.settings.pid_kd);
	temp_sensor.setAmpGain(device_state.settings.tc_amp_gain);
	temp_sensor.setOffset(device_state.settings.tc_offset);
	temp_sensor.setVref(device_state.settings.tc_vref);
	standby_sensor.setDelays(device_state.settings.standby_delay, device_state.settings.off_delay);

	iwdg_set_period_ms(1500);
	iwdg_start();
	
	Task control_task(5_ms, CONTROL_PERIOD, controlTaskFunc);
	control_task.setData(&task_context);
	Task ui_task(5_ms, 100_ms, uiTaskFunc);
	ui_task.setData(&task_context);
	Task debug_task(5_ms, 500_ms, debugTaskFunc);
	debug_task.setData(&task_context);
	Task button_task(1_ms, 5_ms, btnTaskFunc);
	button_task.setData(&task_context);

	Scheduler scheduler;
	scheduler.addTask(ui_task);
	scheduler.addTask(control_task);
	scheduler.addTask(debug_task);
	scheduler.addTask(button_task);

	time::Delay(50_ms).wait();

	while (1) {
		scheduler.runNextTask();
	}

	return 0;
}