#include "TempSensor.h"
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/rcc.h>
#include "Time.h"

// Lookup table for type D thermocouple
// (emf in uV in 10*C increments)
static uint32_t lut_tc_type_d[] = {
    0, 97, 199, 305, 414, 527, 644, 764, 888, 1015,
    1145, 1278, 1414, 1553, 1695, 1840, 1987, 2137, 2289, 2444,
    2602, 2761, 2923, 3086, 3252, 3420, 3590, 3761, 3934, 4109,
    4286, 4464, 4644, 4825, 5007, 5191, 5376, 5563, 5750, 5939,
    6129, 6320, 6512, 6704, 6898, 7093, 7288, 7484, 7681, 7878,
    8076, 8275, 8474, 8674, 8874, 9075, 9276, 9478, 9680, 9883
};

static Temperature tcLookup(uint32_t emf_uv) {
    uint32_t lut_size = sizeof(lut_tc_type_d) / sizeof(uint32_t);
    uint32_t i;
    for (i = 0; i < lut_size - 1; ++i) {
        if (emf_uv < lut_tc_type_d[i]) {
            break;
        }
    }

    uint32_t upper = lut_tc_type_d[i];
    uint32_t lower = lut_tc_type_d[i - 1];
    uint32_t temp = 10 * i + 10 * (emf_uv - lower) / (upper - lower);
    return Temperature{static_cast<int32_t>(temp)};
}

TempSensor::TempSensor(const Config& config)
    : m_adc{config.adc},
      m_channel{config.channel},
      m_amp_gain{config.amp_gain},
      m_vref{config.vref} {  
    rcc_periph_clock_enable(RCC_ADC);
    adc_power_off(m_adc);
    adc_enable_regulator(m_adc);

    time::Delay d(20_us);
    d.wait();
    
    adc_set_clk_source(m_adc, ADC_CLKSOURCE_ADC);
    adc_calibrate(m_adc);
    adc_set_single_conversion_mode(m_adc);
    adc_set_resolution(m_adc, ADC_CFGR1_RES_12_BIT);
    adc_set_right_aligned(m_adc);
    adc_set_sample_time_on_all_channels(m_adc, ADC_SMPTIME_012DOT5);
    uint8_t chseq[] = {m_channel};
    adc_set_regular_sequence(m_adc, 1, chseq);
    adc_power_on(m_adc);
}

void TempSensor::performConversion() {
    adc_start_conversion_regular(m_adc);
    while (!adc_eoc(m_adc));
}

Temperature TempSensor::getTemperature() {
    uint32_t raw = adc_read_regular(m_adc);
    uint32_t tc_voltage = (raw * m_vref.asMillivolts()) / 4096;
    tc_voltage *= 1000;
    tc_voltage /= m_amp_gain;
    return tcLookup(tc_voltage) + m_offset;
}

void TempSensor::setOffset(Temperature offset) {
    m_offset = offset;
}

void TempSensor::setVref(Voltage vref) {
    m_vref = vref;
}

void TempSensor::setAmpGain(uint32_t amp_gain) {
    m_amp_gain = amp_gain;
}
