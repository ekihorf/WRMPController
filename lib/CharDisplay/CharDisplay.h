#pragma once

#include <cstdint>
#include <cstdlib>
#include "I2cDma.h"

class CharDisplay {
public:
    struct Config {
        I2cDma& i2cdma;
        uint8_t i2c_addr;
    }; 

    CharDisplay(Config& config);
    void onOffControl(bool display_on, bool cursor_on, bool blink_on);
    void clear();
    void goTo(uint8_t row, uint8_t column);
    void print(const char *str);
    void printN(const char *str, size_t n);
    void setBacklight(bool on);
    void defineCharacter(uint8_t address, uint8_t character[8]);
    void writeData(uint8_t data);

    void flushBuffer();

private:
    void writeCommand(uint8_t command);
    void i2cWrite(uint8_t data);
    void waitAndFlush();

    bool m_backlight_on = true;
    I2cDma& m_i2cdma;
    uint8_t m_i2c_addr;
    
    uint8_t m_i2c_buf[255];
    uint32_t m_i2c_buf_pos = 0;
};