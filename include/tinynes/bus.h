#ifndef TINYNES_BUS_H
#define TINYNES_BUS_H

#include <array>
#include <cstdint>
#include "tinynes/cpu.h"

namespace tn
{

class Bus
{
public:
    Bus();
    ~Bus() = default;

    void write(uint64_t addr, uint8_t data);
    uint8_t read(uint64_t addr, bool read_only = false);

private:
    CPU cpu_;
    std::array<uint8_t, 64 * 1024> ram_;
};
} // namespace tn

#endif
