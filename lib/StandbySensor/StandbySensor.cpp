#include "StandbySensor.h"

#include <libopencm3/stm32/gpio.h>

StandbySensor::StandbySensor(const Config &config)
: m_port{config.gpio_port}, m_pin{config.gpio_pin}, m_active_low{config.active_low} {
    gpio_mode_setup(m_port, GPIO_MODE_INPUT, m_active_low ? GPIO_PUPD_PULLUP : GPIO_PUPD_PULLDOWN, m_pin);
}

void StandbySensor::setDelays(Duration standby_delay, Duration off_delay) {
    m_standby_delay = standby_delay;
    m_off_delay = off_delay;
}

void StandbySensor::reset() {
    m_activation_time = time::getMsTicks();
}

StandbySensor::State StandbySensor::update() {
    if (!sensorActivated()) {
        m_activation_time = time::getMsTicks();
        return State::Active;
    }

    auto diff = time::getMsTicks() - m_activation_time;
    if (diff > m_off_delay.asMilliseconds()) {
        return State::Off;
    } else if (diff > m_standby_delay.asMilliseconds()) {
        return State::Standby;
    } else {
        return State::Active;
    }
}

bool StandbySensor::sensorActivated() {
    bool r = gpio_get(m_port, m_pin);
    if (m_active_low) {
        return !r;
    } else {
        return r;
    }
}
