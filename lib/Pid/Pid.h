#pragma once

#include <cstdint>
#include <Units.h>

class Pid {
public:
    Pid(Duration interval);
    int32_t calculate(int32_t input, int32_t setpoint);
    void setTunings(int32_t kp, int32_t ki, int32_t kd);
    void setLimits(int32_t min, int32_t max);

private:
    int32_t m_i_term;
    int32_t m_last_input;
    int32_t m_kp = 100;
    int32_t m_ki = 0;
    int32_t m_kd = 0;
    int32_t m_max = 100;
    int32_t m_min = 0;
    uint32_t m_interval_ms;
};