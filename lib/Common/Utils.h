#pragma once

#include <cstdint>
#include <cstdlib>

namespace utils {
    /**
     * @brief Convert unsigned integer to a string. Resulting string is NOT null-terminated.
     * 
     * @param buf output buffer for the string
     * @param val the number to convert
     * @param max_digits maximum number of digits (buffer size)
     * @param pad character used for padding
     */
    void uintToStr(char* buf, uint32_t val, size_t max_digits, char pad = ' ');

    /**
     * @brief Convert signed integer to a string. Resulting string is NON null-terminated.
     * 
     * @param buf output buffer for the string
     * @param val the number to convert
     * @param max_digits maximum number of digits including the minus sign (buffer size)
     * @param pad character used for padding
     */
    void intToStr(char* buf, int32_t val, size_t max_digits, char pad = ' ');
}