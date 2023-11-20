#pragma once

#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <DeviceState.h>
#include <Time.h>
#include <I2cDma.h>
#include <AcControl.h>
#include <CharDisplay.h>
#include <Encoder.h>
#include <TempSensor.h>
#include <Nvs.h>
#include <StandbySensor.h>
#include <Button.h>
#include <Debug.h>

constexpr time::Config time_config {
    .timer = TIM16,
};

constexpr I2cDma::Config i2c_config {
    .i2c = I2C2,
    .dma = DMA1,
    .dma_channel = DMA_CHANNEL1,
    .gpio_port = GPIOA,
    .gpio_pins = GPIO11 | GPIO12,
    .gpio_af = GPIO_AF6
};

constexpr AcControl::Config heater_config {
    .timer = TIM14,
    .timer_oc = TIM_OC1,
    .zero_cross_port = GPIOA,
    .zero_cross_pin = GPIO0,
    .zero_cross_exti = EXTI0,
    .heater_port = GPIOA,
    .heater_pin = GPIO4,
    .heater_pin_af = GPIO_AF4 
};

constexpr CharDisplay::Config disp_config {
    .i2c_addr = 0x27
};

constexpr Encoder::Config encoder_config {
    .timer = TIM3,
    .port_a = GPIOA,
    .pin_a = GPIO6,
    .port_b = GPIOA,
    .pin_b = GPIO7,
    .gpio_af = GPIO_AF1
};

constexpr TempSensor::Config temp_config {
    .adc = ADC1,
    .channel = 8,
    .amp_gain = 340,
    .vref = 3295_mV
};

constexpr Nvs::Config nvs_config {
    .i2c_addr = 0x50,
    .eeprom_size = 1024,
    .lts_size = sizeof(DeviceSettings),
    .sts_start = 64,
};

constexpr StandbySensor::Config standby_config {
    .gpio_port = GPIOA,
    .gpio_pin = GPIO1,
    .active_low = true
};

constexpr Button::Config button_config {
    .gpio_port = GPIOA,
    .gpio_pin = GPIO5,
    .active_low = true
};

constexpr Debug::Config debug_config {
    .usart = USART2,
    .gpio_port = GPIOA,
    .gpio_pins = GPIO2,
    .gpio_af = GPIO_AF1
};