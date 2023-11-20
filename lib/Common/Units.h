#pragma once

#include <cstdint>

class Duration {
public:
    constexpr explicit Duration(uint32_t us) : value{us} {}
    constexpr uint32_t asMicroseconds() { return value; };
    constexpr uint32_t asMilliseconds() { return value / 1'000; };
    constexpr uint32_t asSeconds() { return value / 1'000'000; };

private:
    uint32_t value;
};

constexpr Duration operator "" _us(unsigned long long us) {
    return Duration(static_cast<uint32_t>(us));
}

constexpr Duration operator "" _ms(unsigned long long ms) {
    return Duration(static_cast<uint32_t>(ms) * 1'000);
}

constexpr Duration operator "" _s(unsigned long long s) {
    return Duration(static_cast<uint32_t>(s) * 1'000'000);
}


class Temperature {
public:
    constexpr explicit Temperature(int32_t degrees_c) : value{degrees_c} {}
    constexpr int32_t asDegreesC() {
        return value;
    }

    constexpr Temperature operator-() {
        return Temperature(-value);
    }
    constexpr Temperature operator+(const Temperature& other) const {
        return Temperature(value + other.value);
    }
    constexpr Temperature operator-(const Temperature& other) const {
        return Temperature(value - other.value);
    }
    constexpr bool operator>(const Temperature& other) const {
        return value > other.value;
    }
    constexpr bool operator<(const Temperature& other) const {
        return value < other.value;
    }
    constexpr bool operator>=(const Temperature& other) const {
        return value >= other.value;
    }
    constexpr bool operator<=(const Temperature& other) const {
        return value <= other.value;
    }
    constexpr void operator+=(const Temperature& other) {
        value += other.value;
    }
    constexpr void operator-=(const Temperature& other) {
        value -= other.value;
    }

private:
    int32_t value;
};

constexpr Temperature operator "" _degC(unsigned long long degrees_c) {
    return Temperature(static_cast<int32_t>(degrees_c));
}

class Voltage {
public:
    constexpr explicit Voltage(uint32_t mv) : value{mv} {}
    constexpr uint32_t asMillivolts() {
        return value;
    }

private:
    uint32_t value;
};

constexpr Voltage operator "" _mV(unsigned long long mv) {
    return Voltage(static_cast<uint32_t>(mv));
}