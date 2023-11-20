#include "Nvs.h"
#include <cstring>
#include <libopencm3/stm32/crc.h>
#include "Time.h"

static constexpr size_t PAGE_SIZE{16};
static constexpr Duration WRITE_TIME{5_ms};

Nvs::Nvs(I2cDma& i2cdma, const Config &config)
: m_i2cdma{i2cdma},
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
        if (!m_i2cdma.writeMem(m_i2c_addr | memaddr_high, memaddr_low, page_buf, PAGE_SIZE)) {
            return false;
        }
        
        time::Delay(WRITE_TIME).wait();
    }
    return true;
}

bool Nvs::isBusBusy() {
    return m_i2cdma.isBusy();
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
        m_i2cdma.readMem(m_i2c_addr | memaddr_high, memaddr_low, tmp_buf + memaddr, PAGE_SIZE);
    }

    if (rem) {
        uint8_t memaddr_low = memaddr & 0xFF;
        uint8_t memaddr_high = (memaddr >> 8) & 0x03;
        m_i2cdma.readMem(m_i2c_addr | memaddr_high, memaddr_low, tmp_buf + memaddr, rem);
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
        if (!m_i2cdma.writeMem(m_i2c_addr | memaddr_high, memaddr_low, tmp_buf + memaddr, PAGE_SIZE)) {
            return false;
        }
        time::Delay(WRITE_TIME).wait();
    }

    if (rem) {
        uint8_t memaddr_low = memaddr & 0xFF;
        uint8_t memaddr_high = (memaddr >> 8) & 0x03;
        if (!m_i2cdma.writeMem(m_i2c_addr | memaddr_high, memaddr_low, tmp_buf + memaddr, rem)) {
            return false;
        }
        time::Delay(WRITE_TIME).wait();
    }

    return true;
}

bool Nvs::writeSts(uint32_t value) {
    if (!m_sts_initialized) {
        return false;
    }

    uint32_t sts_size = m_eeprom_size - m_sts_start;
    uint32_t sts_entries = sts_size / 8;
    uint32_t buf[2] = { m_sts_index, value };
    uint32_t memaddr = m_sts_start + m_sts_mem_pos * 8;
    uint8_t memaddr_low = memaddr & 0xFF;
    uint8_t memaddr_high = (memaddr >> 8) & 0x03;

    bool result = m_i2cdma.writeMem(m_i2c_addr | memaddr_high, memaddr_low, reinterpret_cast<uint8_t*>(buf), 8);
    if (!result) {
        return false;
    }

    if (m_sts_index == 0xFFFFFFFF) {
        m_sts_index = 0;
    } else {
        ++m_sts_index;
    }
    
    m_sts_mem_pos = (m_sts_mem_pos + 1) % sts_entries;
    return true;
}

etl::optional<uint32_t> Nvs::readSts() {
    uint32_t sts_size = m_eeprom_size - m_sts_start;
    uint32_t pages = sts_size / PAGE_SIZE;
    uint32_t entries = sts_size / 8;

    // this array will be huge but we have a lot of free RAM so it should be fine...
    uint8_t mem_buf[sts_size];
    uint32_t* mem_buf4 = reinterpret_cast<uint32_t*>(mem_buf);

    uint32_t offset = 0;
    for (uint32_t i = 0; i < pages; ++i, offset += PAGE_SIZE) {
        uint32_t memaddr = m_sts_start + offset;
        uint8_t memaddr_low = memaddr & 0xFF;
        uint8_t memaddr_high = (memaddr >> 8) & 0x03;
        m_i2cdma.readMem(m_i2c_addr | memaddr_high, memaddr_low, mem_buf + offset, 16);
    }

    uint32_t max_ind = 0;
    uint32_t max_ind_val = 0;
    bool found = false;
    for (uint32_t i = 0; i < entries; ++i) {
        uint32_t cur_ind = mem_buf4[i * 2];
        uint32_t cur_val = mem_buf4[i * 2 + 1];

        if (cur_ind == 0xFFFFFFFF || cur_ind <= max_ind) {
            break;
        }

        m_sts_index = max_ind = cur_ind;
        m_sts_mem_pos = i;
        max_ind_val = cur_val; 
        found = true;
    } 

    m_sts_initialized = true;

    if (found) {
        return etl::optional<uint32_t>(max_ind_val);
    } else {
        return etl::optional<uint32_t>();
    }
}
