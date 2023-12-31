#include "Pid.h"

constexpr int32_t mul = 1000;

Pid::Pid(Duration interval) : m_interval_ms{interval.asMilliseconds()} {}

int32_t Pid::calculate(int32_t input, int32_t setpoint) {
    int32_t error = setpoint - input;
    m_i_term += (m_ki * error);
    if (m_i_term > m_max * mul) {
        m_i_term = m_max * mul;
    } else if (m_i_term < (m_min * mul)) {
        m_i_term = m_min * mul;
    }

    int32_t d_input = error - m_last_error;
    m_last_error = error;

    int32_t output = (m_kp * error + m_i_term + m_kd * d_input) / mul;
    if (output > m_max) {
        output = m_max;
    } else if (output < m_min)  {
        output = m_min;
    }

    return output;
}

void Pid::setTunings(int32_t kp, int32_t ki, int32_t kd) {
    m_i_term = 0;
    m_kp = kp;
    m_ki = ki * m_interval_ms / 1000;
    m_kd = kd * 1000 / m_interval_ms;
}

void Pid::setLimits(int32_t min, int32_t max) {
    m_min = min;
    m_max = max;
}