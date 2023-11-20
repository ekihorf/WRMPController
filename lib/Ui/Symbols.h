#pragma once

#include <stdint.h>

namespace ui {
    constexpr uint8_t SYMBOLS_DATA[2][8] = {
        {
            0b00100,
            0b01010,
            0b01010,
            0b01010,
            0b01010,
            0b10001,
            0b10001,
            0b01110
        },
        {
            0b00000,
            0b01110,
            0b10011,
            0b10101,
            0b10001,
            0b01110,
            0b00000,
            0b00000
        }
    };

    enum class Symbols: uint8_t {
        CurrentTemperature = 1,
        SetTemperature,
    };
}