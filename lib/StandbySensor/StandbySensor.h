#pragma once

#include <cstdint>
#include <etl/utility.h>
#include <etl/optional.h>
#include <Time.h>

class StandbySensor {
public:
    struct Config {
        uint32_t gpio_port;
        uint16_t gpio_pin;
        bool active_low;
    };

    enum class State {
        Active,
        Standby,
        Off
    };

    StandbySensor(const Config& config);
    void setDelays(Duration standby_delay, Duration off_delay);
    void reset();
    State update();
    bool isHandleInStand();

private:
    uint32_t m_port;
    uint16_t m_pin;
    bool m_active_low;
    Duration m_standby_delay{5_s};
    Duration m_off_delay{10_s};
    uint64_t m_activation_time;
    bool m_handle_in_stand;

    bool sensorActivated();
};