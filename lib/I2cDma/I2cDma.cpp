#include "I2cDma.h"
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/dmamux.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/assert.h>
#include "Irqs.h"

I2cDma::I2cDma(const Config &config)
: m_i2c{config.i2c},
  m_dma{config.dma},
  m_dma_channel{config.dma_channel} {
	i2c_peripheral_disable(m_i2c);
	i2c_enable_analog_filter(m_i2c);
	i2c_set_digital_filter(m_i2c, 0);
	i2c_set_speed(m_i2c, i2c_speed_sm_100k, rcc_apb1_frequency / 1'000'000);
	i2c_set_7bit_addr_mode(m_i2c);
	i2c_peripheral_enable(m_i2c);

    uint8_t request_id;
    if (m_i2c == I2C1) {
        request_id = DMAMUX_CxCR_DMAREQ_ID_I2C1_TX;
    } else if (m_i2c == I2C2) {
        request_id = DMAMUX_CxCR_DMAREQ_ID_I2C2_TX;
    } else {
        cm3_assert(false);
    }

    dmamux_set_dma_channel_request(DMAMUX1, m_dma_channel, request_id);

    Irqs::getDmaHandler(m_dma, m_dma_channel).connect<&I2cDma::dmaIsr>(this);
    dma_set_priority(m_dma, m_dma_channel, DMA_CCR_PL_LOW);
    dma_set_memory_size(m_dma,m_dma_channel, DMA_CCR_MSIZE_8BIT);
    dma_set_peripheral_size(m_dma, m_dma_channel, DMA_CCR_PSIZE_8BIT);
    dma_enable_memory_increment_mode(m_dma, m_dma_channel);
    dma_set_read_from_memory(m_dma, m_dma_channel);
    dma_enable_transfer_complete_interrupt(m_dma, m_dma_channel);
}

void I2cDma::startDmaWrite(uint8_t i2c_addr, uint8_t data[], uint8_t count) {
    i2c_set_7bit_address(m_i2c, i2c_addr);
    i2c_set_write_transfer_dir(m_i2c);
    i2c_set_bytes_to_transfer(m_i2c, count);
    i2c_enable_autoend(m_i2c);
    dma_set_memory_address(m_dma, m_dma_channel, (uint32_t)data);
    dma_set_peripheral_address(m_dma, m_dma_channel, (uint32_t)&I2C_TXDR(m_i2c));
    dma_set_number_of_data(m_dma, m_dma_channel, count);
    dma_enable_channel(m_dma, m_dma_channel);
    i2c_enable_txdma(m_i2c);
    i2c_send_start(m_i2c);
    m_busy = true;
}

bool I2cDma::writeMem(uint8_t i2c_addr, uint8_t mem_addr, uint8_t buf[], uint8_t count) {
    i2c_disable_txdma(m_i2c);
    i2c_set_7bit_address(m_i2c, i2c_addr);
    i2c_set_write_transfer_dir(m_i2c);
    i2c_set_bytes_to_transfer(m_i2c, count+1);
    i2c_enable_autoend(m_i2c);
    i2c_send_start(m_i2c);

    if (!sendByte(mem_addr)) {
        return false;
    }

    while (count--) {
        if (!sendByte(*buf++)) {
            return false;
        }
    }

    return true;
}

void I2cDma::readMem(uint8_t i2c_addr, uint8_t mem_addr, uint8_t buf[], uint8_t count) {
    i2c_disable_txdma(m_i2c);
    i2c_transfer7(m_i2c, i2c_addr, &mem_addr, 1, buf, count);
}

bool I2cDma::isBusy() {
    return m_busy;
}

void I2cDma::dmaIsr() {
    dma_clear_interrupt_flags(m_dma, m_dma_channel, DMA_TCIF);
    dma_disable_channel(m_dma, m_dma_channel);
    i2c_clear_stop(m_i2c);
    m_busy = false;
}

bool I2cDma::sendByte(uint8_t byte) {
    bool wait = true;
    while (wait) {
            if (i2c_transmit_int_status(m_i2c)) {
                    wait = false;
            }
            if (i2c_nack(m_i2c)) {
                return false;
            }
    }
    i2c_send_data(m_i2c, byte);
    return true;
}
