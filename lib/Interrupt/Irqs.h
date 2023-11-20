#pragma once

#include <cstdint>
#include "InterruptHandler.h"

class Irqs {
public:
    static InterruptHandler& getExtiHandler(uint32_t exti);
};