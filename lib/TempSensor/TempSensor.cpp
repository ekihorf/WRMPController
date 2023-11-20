#include "TempSensor.h"
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/rcc.h>
#include "SysTick.h"

TempSensor::TempSensor(uint32_t adc, uint32_t channel) : m_adc{adc}, m_channel{channel} {
    rcc_periph_clock_enable(RCC_ADC);
    adc_power_off(m_adc);
    adc_enable_regulator(m_adc);

    // wait for the regulator 
    // TODO: this delay is way too long (10us should be enough)
    systick::Delay d(1);
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

void TempSensor::startConversion() {
    adc_start_conversion_regular(m_adc);
}

uint32_t TempSensor::getTemperature() {
    return adc_read_regular(m_adc);
}
