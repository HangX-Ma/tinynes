#ifndef TINYNES_BUS_H
#define TINYNES_BUS_H

#include <array>
#include <cstdint>

#include "tinynes/cpu.h"
#include "tinynes/ppu.h"
#include "tinynes/cartridge.h"

namespace tn
{

class Bus
{
public:
    Bus();
    ~Bus() = default;

    void cpuWrite(uint64_t addr, uint8_t data);
    uint8_t cpuRead(uint64_t addr, bool read_only = false);

public:
    // system interfaces
    void insertCartridge(const std::shared_ptr<Cartridge> &cartridge);
    void clock();
    void reset();

public:
    // access device on the bus
    CPU &cpu() { return cpu_; }
    PPU &ppu() { return ppu_; }
    auto &cpuRAM() { return cpu_ram_; }
    auto cartridge() { return cart_; }
    auto &controller() { return controller_; }

private:
    CPU cpu_; // 6052 CPU
    PPU ppu_; // 2C02 PPU
    std::array<uint8_t, 2048> cpu_ram_;
    std::shared_ptr<Cartridge> cart_;
    // controller
    uint8_t controller_[2];
    uint8_t controller_state_[2];

    // record elapsed clock ticks
    uint64_t sys_clock_counter_{0};
};
} // namespace tn

#endif
