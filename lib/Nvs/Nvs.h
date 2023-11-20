#pragma once

#include "I2cDma.h"
#include <etl/optional.h>

class Nvs {
public:
    struct Config {
        /* I2C driver */
        I2cDma& i2c;
        /* I2C address */
        uint8_t i2c_addr;
        /* 24C EEPROM size in bytes */
        uint32_t eeprom_size;

        /* 
            Long Term Storage contains data that is not updated often (device settings).
            It is placed at the beginning of the EEPROM, and doesn't implement wear levelling,
            so proper care must be taken to not exceed the write limit.

            Read and write functions use CRC to check the validity of stored data. Keep in
            mind that the CRC is stored in the last 4 bytes after the actual data, so
            for example, if lts_size is set to 32, then 36 bytes are written to the EEPROM.
        */
        /* Long Term Storage size (max 60 bytes, multiple of 4 bytes) */
        uint32_t lts_size;

        /*
            Short Term Storage is a 4 byte value that is written using a wear levelling
            mechanism. This can be used to store data that is changed often
            (like set iron temperature)
        */
        /* Short Term Storage start (must be aligned to page size) */
        uint32_t sts_start;
    };

    Nvs(Config& config);
    bool erase();
    bool isBusBusy();
    bool readLts(void* buf);
    bool writeLts(const void* buf);
    etl::optional<uint32_t> readSts();
    bool writeSts(uint32_t value);

private:
    I2cDma& m_i2c;
    uint8_t m_i2c_addr;
    uint32_t m_eeprom_size;
    uint32_t m_lts_size;
    uint32_t m_sts_start;
};