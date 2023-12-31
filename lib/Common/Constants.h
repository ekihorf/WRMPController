#pragma once

#include "Units.h"

constexpr Temperature MAX_TIP_TEMP = 450_degC;
constexpr Temperature MIN_TIP_TEMP = 100_degC;

constexpr int32_t MAX_HEATER_POWER = 90;
constexpr Duration CONTROL_PERIOD = 200_ms;

namespace defaults {
    constexpr Temperature SET_TEMP = 200_degC;
    constexpr DeviceSettings SETTINGS = {
        .temp_increment = 5_degC,
        .standby_temp = 150_degC,
        .standby_delay = 60_s,
        .off_delay = 300_s,
        .pid_kp = 7000,
        .pid_ki = 1700,
        .pid_kd = 40,
        .tc_vref = 3295_mV,
        .tc_amp_gain = 385,
        .tc_offset = 10_degC
    };
}