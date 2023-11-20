#pragma once

#include <cstdint>
#include "Units.h"

enum class HeatingStatus {
    Off,
    On,
    Standby
};

struct DeviceState {
    Celsius& set_temp;
    Celsius& tip_temp;
    Celsius& standby_temp;
    uint32_t& heater_power;
    HeatingStatus& heating_status;

    uint32_t& temp_increment;
};