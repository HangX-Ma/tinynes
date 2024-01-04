#include "tinynes/bus.h"

namespace tn
{

Bus::Bus()
{
    // connect CPU to main Bus
    cpu_.connectBus(this);
    // clear RAM at init
    for (auto &mem : ram_) {
        mem = 0x00;
    }
}

void Bus::write(uint64_t addr, uint8_t data)
{
    if (addr >= 0 && addr <= 0xFFFF) {
        ram_[addr] = data;
    }
}
uint8_t Bus::read(uint64_t addr, bool read_only /*unused*/)
{
    if (addr >= 0 && addr <= 0xFFFF) {
        return ram_[addr];
    }
}

} // namespace tn