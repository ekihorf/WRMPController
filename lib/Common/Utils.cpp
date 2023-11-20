#include "Utils.h"

void utils::uintToStr(char* buf, uint32_t val, size_t max_digits) {
    for (int i = max_digits - 1; i >= 0; --i) {
        buf[i] = val % 10 + '0';
        val /= 10;
    } 
}