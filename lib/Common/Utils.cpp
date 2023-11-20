#include "Utils.h"

void utils::uintToStr(char* buf, uint32_t val, size_t width, char pad) {
    for (int32_t i = width - 1; i >= 0; --i) {
        uint32_t digit = val % 10;
        if (digit) {
            buf[i] = digit + '0';
        } else {
            buf[i] = (val || (static_cast<uint32_t>(i) == width - 1)) ? '0' : pad;
        }
        val /= 10;
    } 
}

void utils::intToStr(char *buf, int32_t val, size_t width, char pad) {
    uint32_t aval = abs(val);
    bool minus = val < 0;
    for (int32_t i = width - 1; i >= 0; --i) {
        uint32_t digit = aval % 10;
        if (digit) {
            buf[i] = digit + '0';
        } else {
            if (aval || (static_cast<uint32_t>(i) == width - 1)) {
                buf[i] = '0';
            } else {
                buf[i] = minus ? '-' : pad;
                minus = false;
            }
        }
        aval /= 10;
    } 
}
