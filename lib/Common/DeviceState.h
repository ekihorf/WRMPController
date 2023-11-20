#pragma once

#include <cstdint>
#include "Units.h"

enum class HeatingStatus {
    Off,
    On,
    Standby
};

struct DeviceState {
    Temperature set_temp;
    Temperature tip_temp;
    Temperature standby_temp;
    uint32_t heater_power;
    HeatingStatus heating_status;

    Temperature temp_increment;
};