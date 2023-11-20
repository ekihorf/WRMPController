#pragma once
#include "Units.h"

class Button {

public:
    enum class EventType {
        None,
        ShortPress,
        LongPress
    };

    struct Config {
        uint32_t gpio_port;
        uint32_t gpio_pin;
        bool active_low;
    };
    
    Button(const Config& config);
    EventType update();
    bool isPressedRaw();

private:
    enum class State {
        Initial,
        Debounce,
        Pressed,
        WaitForRelease
    };

    uint32_t m_port;
    uint16_t m_pin;
    bool m_active_low = false;
    State m_state = State::Initial;
    uint32_t m_debounce_start;
};