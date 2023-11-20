#include "Nvs.h"
#include <cstring>
#include <libopencm3/stm32/crc.h>
#include "Time.h"

static constexpr size_t PAGE_SIZE{16};
static constexpr Duration WRITE_TIME{5_ms};

Nvs::Nvs(Config &config)
: m_i2c{config.i2c},
  m_i2c_addr{config.i2c_addr},
  m_eeprom_size{config.eeprom_size},
  m_lts_size{config.lts_size},
  m_sts_start{config.sts_start} {

}

bool Nvs::erase() {
    if (isBusBusy()) {
        return false;
    }
    
    uint8_t page_buf[PAGE_SIZE];
    size_t pages = m_eeprom_size / PAGE_SIZE;
    memset(page_buf, 0xFF, PAGE_SIZE);

    for (size_t i = 0; i < pages; ++i) {
        uint32_t memaddr = i * PAGE_SIZE;
        uint8_t memaddr_low = memaddr & 0xFF;
        uint8_t memaddr_high = (memaddr >> 8) & 0x03;
        if (!m_i2c.writeMem(m_i2c_addr | memaddr_high, memaddr_low, page_buf, PAGE_SIZE)) {
            return false;
        }
        
        time::Delay(WRITE_TIME).wait();
    }
    return true;
}

bool Nvs::isBusBusy() {
    return m_i2c.isBusy();
}

bool Nvs::readLts(void *buf) {
    if (isBusBusy()) {
        return false;
    }
    uint8_t tmp_buf[64] = {0};
    size_t size = m_lts_size + 4;
    size_t pages = size / PAGE_SIZE;
    size_t rem = size % PAGE_SIZE;
    uint32_t memaddr = 0;

    for (size_t i = 0; i < pages; ++i, memaddr += PAGE_SIZE) {
        uint8_t memaddr_low = memaddr & 0xFF;
        uint8_t memaddr_high = (memaddr >> 8) & 0x03;
        m_i2c.readMem(m_i2c_addr | memaddr_high, memaddr_low, tmp_buf + memaddr, PAGE_SIZE);
    }

    if (rem) {
        uint8_t memaddr_low = memaddr & 0xFF;
        uint8_t memaddr_high = (memaddr >> 8) & 0x03;
        m_i2c.readMem(m_i2c_addr | memaddr_high, memaddr_low, tmp_buf + memaddr, rem);
    }

    crc_reset();
    uint32_t crc = crc_calculate_block(reinterpret_cast<uint32_t*>(tmp_buf), m_lts_size / 4);

    uint32_t stored_crc = *reinterpret_cast<uint32_t*>(tmp_buf + m_lts_size);
    if (crc != stored_crc) {
        return false;
    }

    memcpy(buf, tmp_buf, m_lts_size);
    return true;
}

bool Nvs::writeLts(const void *buf) {
    if (isBusBusy()) {
        return false;
    }
    uint8_t tmp_buf[64];
    memcpy(tmp_buf, buf, m_lts_size);

    crc_reset();
    uint32_t crc = crc_calculate_block(reinterpret_cast<uint32_t*>(tmp_buf), m_lts_size / 4);
    *reinterpret_cast<uint32_t*>(tmp_buf + m_lts_size) = crc;

    size_t size = m_lts_size + 4;
    size_t pages = size / PAGE_SIZE;
    size_t rem = size % PAGE_SIZE;
    uint32_t memaddr = 0;

    for (size_t i = 0; i < pages; ++i, memaddr += PAGE_SIZE) {
        uint8_t memaddr_low = memaddr & 0xFF;
        uint8_t memaddr_high = (memaddr >> 8) & 0x03;
        if (!m_i2c.writeMem(m_i2c_addr | memaddr_high, memaddr_low, tmp_buf + memaddr, PAGE_SIZE)) {
            return false;
        }
        time::Delay(WRITE_TIME).wait();
    }

    if (rem) {
        uint8_t memaddr_low = memaddr & 0xFF;
        uint8_t memaddr_high = (memaddr >> 8) & 0x03;
        if (!m_i2c.writeMem(m_i2c_addr | memaddr_high, memaddr_low, tmp_buf + memaddr, rem)) {
            return false;
        }
        time::Delay(WRITE_TIME).wait();
    }

    return true;
}

bool Nvs::writeSts(uint32_t value) {
    return false;
}

etl::optional<uint32_t> Nvs::readSts() {
    return etl::optional<uint32_t>();
}
