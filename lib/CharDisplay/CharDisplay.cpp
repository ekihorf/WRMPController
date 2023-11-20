#include "CharDisplay.h"
#include <libopencm3/stm32/i2c.h>
#include "Time.h"

static constexpr uint8_t Backlight_bit = 0x08;
static constexpr uint8_t Rs_bit = 0x01;
static constexpr uint8_t E_bit = 0x04;

CharDisplay::CharDisplay(Config& config)
: m_i2cdma{config.i2cdma},
  m_i2c_addr{config.i2c_addr} {
    i2cWrite(0x00);
    waitAndFlush();
    for (int i = 0; i < 3; ++i) {
        // function set: 8 bit mode
        i2cWrite(0x30 | E_bit);
        i2cWrite(0x30);
        waitAndFlush();
        time::Delay(5_ms).wait();
    }

    i2cWrite(0x20 | E_bit);
    i2cWrite(0x20);
    waitAndFlush();

    // function set: 4-bit mode, 2 line, 5x8 font
    writeCommand(0x28);

    onOffControl(false, false, false);
    clear();

    // entry mode set: increment, no shift
    writeCommand(0x06);

    onOffControl(true, false, false);
}

void CharDisplay::onOffControl(bool display_on, bool cursor_on, bool blink_on) {
    uint8_t command = 0x08;
    if (display_on) {command |= 0x04;}
    if (cursor_on) {command |= 0x02;}
    if (blink_on) {command |= 0x01;}
    writeCommand(command);
}

void CharDisplay::clear() {
    writeCommand(0x01);
    waitAndFlush();
    time::Delay(3_ms).wait();
}

void CharDisplay::goTo(uint8_t row, uint8_t column) {
    writeCommand(0x80 | (column + 0x40 * row));
}

void CharDisplay::print(char *str) {
    while (*str) {
        writeData(*str);
        ++str;
    }
}

void CharDisplay::printN(char *str, size_t n) {
    for (size_t i = 0; i < n && *str; ++i, ++str) {
        writeData(*str);
    }
}

void CharDisplay::setBacklight(bool on) {
    m_backlight_on = on;
    i2cWrite(0x00);
}

// Store custom character in CGRAM
// Valid addresses: 0x00 - 0x07
// Character format is defined in HD44780 datasheet
void CharDisplay::defineCharacter(uint8_t address, uint8_t character[8]) {
    writeCommand(0x40 | address * 8);
    for (int i = 0; i < 8; ++i) {
        writeData(character[i]);
    }
}

void CharDisplay::writeCommand(uint8_t command) {
    i2cWrite((command & 0xF0) | E_bit);
    i2cWrite(command & 0xF0);
    i2cWrite((command << 4) | E_bit);
    i2cWrite(command << 4);
}

void CharDisplay::writeData(uint8_t data) {
    i2cWrite((data & 0xF0) | E_bit | Rs_bit);
    i2cWrite(data & 0xF0);
    i2cWrite((data << 4) | E_bit | Rs_bit);
    i2cWrite(data << 4);
}

void CharDisplay::flushBuffer() {
    if (m_i2c_buf_pos == 0) { return; };

    if (!m_i2cdma.isBusy()) {
        m_i2cdma.startWrite(m_i2c_addr, m_i2c_buf, m_i2c_buf_pos);
        m_i2c_buf_pos = 0;
    }

}

void CharDisplay::i2cWrite(uint8_t data) {
    data &= 0xF7;
    if (m_backlight_on) {
        data |= Backlight_bit;
    }

    // i2c_transfer7(m_i2c, m_i2c_addr, &data, 1, nullptr, 0);
    // time::Delay(10_us).wait();    

    if (m_i2c_buf_pos < 255) {
        m_i2c_buf[m_i2c_buf_pos++] = data;
    }
}

void CharDisplay::waitAndFlush()
{
    while (m_i2cdma.isBusy())
        ;
    flushBuffer();
}
