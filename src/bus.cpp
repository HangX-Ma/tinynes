#include "tinynes/bus.h"
#include "spdlog/spdlog.h"

namespace tn
{

Bus::Bus()
{
    // connect CPU to main Bus
    cpu_.connectBus(this);
    // clear RAM at init
    for (auto &mem : cpu_ram_) {
        mem = 0x00;
    }
}

void Bus::cpuWrite(uint64_t addr, uint8_t data)
{
    if (cart_->cpuWrite(addr, data)) {
        // placeholder for future extension
    }
    else if (addr >= 0 && addr <= 0x1FFF) {
        // system internal RAM address range. The range covers 8KB, though
        // there is only 2KB available. That 2KB is "mirrored" through this address range.
        cpu_ram_[addr & 0x07FF /*2KB*/] = data;
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // PPU registers address range, mirrored every 8 bytes
        ppu_.cpuWrite(addr & 0x0007, data);
    }
    // spdlog::warn("{}: RAM receives illegal address {:#04x}", __func__, addr);
}

uint8_t Bus::cpuRead(uint64_t addr, bool read_only /*unused*/)
{
    uint8_t data = 0x00;

    // cartridge  access
    if (cart_->cpuRead(addr, data)) {
    }
    // system internal RAM address range, mirrored every 2048 bytes
    else if (addr >= 0 && addr <= 0x1FFF) {
        return cpu_ram_[addr & 0x07FF /*2KB*/];
    }
    // PPU registers address range, mirrored every 8 bytes
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        data = ppu_.cpuRead(addr & 0x0007, read_only);
    }

    // spdlog::warn("{}: RAM receives illegal address {:#04x}", __func__, addr);
    return 0;
}

void Bus::insertCartridge(const std::shared_ptr<Cartridge> &cartridge)
{
    cart_ = cartridge;
    ppu_.connectCartridge(cartridge);
}

void Bus::clock()
{

    ppu_.clock();

    if (sys_clock_counter_ % 3 == 0) {
        cpu_.clock();
    }

    sys_clock_counter_ += 1;
}

void Bus::reset()
{
    cpu_.reset();
    sys_clock_counter_ = 0;
}

} // namespace tn