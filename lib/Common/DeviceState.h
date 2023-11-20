#pragma once

#include <cstdint>
#include "Units.h"

enum class HeatingStatus {
    Off,
    On,
    Standby
};

struct DeviceSettings {
    Temperature set_temp;
    Temperature temp_increment;
    Temperature standby_temp;
    Duration standby_delay;
    Duration off_delay;
    int32_t pid_kp;
    int32_t pid_ki;
    int32_t pid_kd;
    Voltage tc_vref;
    uint32_t tc_amp_gain;
};

struct DeviceState {
    Temperature set_temp;
    Temperature tip_temp;
    Temperature standby_temp;
    Duration standby_delay;
    uint32_t heater_power;
    HeatingStatus heating_status;

    Temperature temp_increment;
};
