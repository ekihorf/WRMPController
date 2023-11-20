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
    constexpr explicit Celsius(int32_t degrees) : value{degrees} {}
    constexpr Celsius operator-() {
        return Celsius(-value);
    }
    constexpr Celsius operator+(Celsius& other) {
        return Celsius(value + other.value);
    }
    constexpr Celsius operator-(Celsius& other) {
        return Celsius(value - other.value);
    }

    int32_t value;
};

constexpr Celsius operator "" _degC(unsigned long long degrees) {
    return Celsius(static_cast<int32_t>(degrees));
}

class Millivolts {
public:
    constexpr explicit Millivolts(uint32_t mv) : value{mv} {}

    uint32_t value;
};

constexpr Millivolts operator "" _mV(unsigned long long mv) {
    return Millivolts(static_cast<uint32_t>(mv));
}