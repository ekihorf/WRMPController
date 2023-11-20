#include "Button.h"
#include "Time.h"
#include <libopencm3/stm32/gpio.h>

static constexpr uint32_t DEBOUNCE_TIME_MS = 10;
static constexpr uint32_t LONG_PRESS_TIME_MS = 1500;
static constexpr uint32_t SHORT_PRESS_TIME_MS = 50;

Button::Button(uint32_t port, uint16_t pin, bool active_low)
: m_port{port},
  m_pin{pin},
  m_active_low{active_low} {
    uint8_t pull = active_low ? GPIO_PUPD_PULLUP : GPIO_PUPD_PULLDOWN;
    gpio_mode_setup(port, GPIO_MODE_INPUT, pull, pin);
}

Button::EventType Button::update() {
    uint32_t current_time = time::getMsTicks();
    bool pressed = gpio_get(m_port, m_pin);
    if (m_active_low) {
        pressed = !pressed;
    }

    switch (m_state) {
    case State::Initial:
        if (pressed) {
            m_state = State::Debounce;
            m_debounce_start = current_time;
        }
        break;

    case State::Debounce:
        if (pressed) {
            if (current_time - m_debounce_start > DEBOUNCE_TIME_MS) {
                m_state = State::Pressed;
            }
        } else {
            m_state = State::Initial; 
        }
        break;

    case State::Pressed:
        if (pressed) {
            if (current_time - m_debounce_start > LONG_PRESS_TIME_MS) {
                m_state = State::WaitForRelease;
                return EventType::LongPress;
            }
        } else {
            if (current_time - m_debounce_start > SHORT_PRESS_TIME_MS) {
                m_state = State::WaitForRelease;
                return EventType::ShortPress;
            } else {
                m_state = State::Initial;
            }
        }
        break;

    case State::WaitForRelease:
        if (!pressed) {
            m_state = State::Initial;
        }
        break;
    }

    return EventType::None;
}
