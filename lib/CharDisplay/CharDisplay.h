#pragma once

#include <cstdint>

class CharDisplay {
public:
    CharDisplay(uint32_t i2c, uint8_t i2c_addr);
    void onOffControl(bool display_on, bool cursor_on, bool blink_on);
    void clear();
    void goTo(uint8_t row, uint8_t column);
    void print(char *str);
    void setBacklight(bool on);
    void defineCharacter(uint8_t address, uint8_t character[8]);
    void writeData(uint8_t data);

private:
    void writeCommand(uint8_t command);
    void i2cWrite(uint8_t data);

    bool m_backlight_on = true;
    uint32_t m_i2c;
    uint8_t m_i2c_addr;
};