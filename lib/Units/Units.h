#pragma once

#include <cstdint>

class Microsecond {
public:
    constexpr explicit Microsecond(uint32_t us) : value{us} {}

    uint32_t value;
};

constexpr Microsecond operator "" _us(unsigned long long us) {
    return Microsecond(static_cast<uint32_t>(us));
}

constexpr Microsecond operator "" _ms(unsigned long long ms) {
    return Microsecond(static_cast<uint32_t>(ms) * 1000);
}


class Celsius {
public:
    constexpr explicit Celsius(uint32_t degrees) : value{degrees} {}

    uint32_t value;
};

constexpr Celsius operator "" _C(unsigned long long degrees) {
    return Celsius(static_cast<uint32_t>(degrees));
}