#include "AcControl.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>

AcControl::AcControl(uint32_t zero_port, uint16_t zero_pin, uint32_t out_port, uint16_t out_pin) :
        m_zero_pin{zero_pin},
        m_zero_port{zero_port},
        m_out_pin{out_pin},
        m_out_port{out_port} {}

void AcControl::setControlPeriod(uint32_t halfcycles) {
    m_control_period = halfcycles;
}

void AcControl::setMainsFrequency(uint32_t freq_hz) {
    m_mains_frequency = freq_hz;
}

void AcControl::setOnHalfcycles(uint32_t on_halfcycles) {
    m_duty_cycle = on_halfcycles;
    m_status = Status::Running;
}

void AcControl::update(uint32_t current_ticks) {
    // skip the check if this is a first call
    if (m_previous_ticks != 0) {
        uint32_t required_halfcycles = ((current_ticks - m_previous_ticks) * m_mains_frequency / 500);

        // if this is smaller, the time between updates was too short to make this check
        if (required_halfcycles > 1) {
            // time measurement might be not accurate, so expect smaller number of halfcycles
            required_halfcycles -= 1;
            if (m_halfcycles_since_update < required_halfcycles) {
                gpio_clear(m_out_port, m_out_pin);
                m_duty_cycle = 0;
                m_status = Status::ZeroDetectorFault;
            }
        }
    }

    m_halfcycles_since_update = 0;
    m_previous_ticks = current_ticks;
}

void AcControl::signalZeroCrossing() {
    ++m_halfcycles_since_update;
    // called from the interrupt handler; to be removed later when there is a better solution
    if (m_current_halfcycle++ < m_duty_cycle) {
        gpio_set(m_out_port, m_out_pin);
    } else {
        gpio_clear(m_out_port, m_out_pin);
    }
    if (m_current_halfcycle >= m_control_period) {
        m_current_halfcycle = 0;
    }
}

AcControl::Status AcControl::getStatus() {
    return m_status;
}