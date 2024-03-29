#ifndef TINYNES_BUS_H
#define TINYNES_BUS_H

#include <array>
#include <cstdint>

#include "tinynes/cpu.h"
#include "tinynes/ppu.h"
#include "tinynes/apu.h"
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
    bool clock(); // return if sound thread generates a new value
    void reset();

public:
    // access device on the bus
    CPU &cpu() { return cpu_; }
    PPU &ppu() { return ppu_; }
    APU &apu() { return apu_; }
    auto &cpuRAM() { return cpu_ram_; }
    auto cartridge() { return cart_; }
    auto &controller() { return controller_; }

    // APU
    void setAudioSampleFrequency(uint32_t sample_rate);
    double getAudioSample() { return audio_sample_; }
    void setAudioSample(double val) { audio_sample_ = val; }

private:
    // accumulated audio sample time between two system sample time
    double audio_time_{0.0};
    // output audio sample value
    double audio_sample_{0.0};
    double audio_time_in_sys_sample_{0.0};
    double audio_time_in_nes_clock_{0.0};

private:
    CPU cpu_; // 6052 CPU
    PPU ppu_; // 2C02 PPU
    APU apu_; // 2A03 APU
    std::array<uint8_t, 2048> cpu_ram_;
    std::shared_ptr<Cartridge> cart_;
    // controller
    uint8_t controller_[2];
    uint8_t controller_state_[2];

    // record elapsed clock ticks
    uint64_t sys_clock_counter_{0};

private:
    uint8_t dma_page_{0x00};
    uint8_t dma_addr_{0x00};
    uint8_t dma_data_{0x00};

    // DMA transfers need to be timed accurately. In principle it takes
    // 512 cycles to read and write the 256 bytes of the OAM memory, a
    // read followed by a write. However, the CPU needs to be on an "even"
    // clock cycle, so a dummy cycle of idleness may be required
    bool dma_dummy_{true};

    //  flag to indicate that a DMA transfer is happening
    bool dma_transfer_{false};
};
} // namespace tn

#endif
