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
    // APU registers are mapped in range $4000-$4013, $4015 and $4017
    else if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015 || addr == 0x4017) {
        apu_.cpuWrite(addr, data);
    }
    else if (addr == 0x4014) {
        // A write to this address initiates a DMA transfer
        dma_page_ = data;
        dma_addr_ = 0x00;
        dma_transfer_ = true;
    }
    else if (addr >= 0x4016 && addr <= 0x4017) {
        controller_state_[addr & 0x0001] = controller_[addr & 0x0001];
    }
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
    // No need to add APU read because synchronization necessity
    else if (addr >= 0x4016 && addr <= 0x4017) {
        data = static_cast<uint8_t>((controller_state_[addr & 0x0001] & 0x80) > 0);
        controller_state_[addr & 0x0001] <<= 1;
    }
    return data;
}

void Bus::insertCartridge(const std::shared_ptr<Cartridge> &cartridge)
{
    cart_ = cartridge;
    ppu_.connectCartridge(cartridge);
}

void Bus::reset()
{
    cart_->reset();
    cpu_.reset();
    ppu_.reset();
    sys_clock_counter_ = 0;
}

bool Bus::clock()
{

    ppu_.clock();
    apu_.clock();

    if (sys_clock_counter_ % 3 == 0) {
        // DMA start?
        if (dma_transfer_) {
            // wait until the next even CPU clock cycle before it starts
            if (dma_dummy_) {
                if (sys_clock_counter_ % 2 == 1) {
                    dma_dummy_ = false;
                }
            }
            else {
                // DMA can take place!
                if (sys_clock_counter_ % 2 == 0) {
                    // On even clock cycles, read from CPU bus
                    dma_data_ = cpuRead(dma_page_ << 8 | dma_addr_);
                }
                else {
                    // On odd clock cycles, write to PPU OAM
                    ppu_.oam()[dma_addr_] = dma_data_;
                    dma_addr_ += 1;
                    // If this wraps around, we know that 256 bytes have been written, so end the
                    // DMA transfer, and proceed as normal
                    if (dma_addr_ == 0x00) {
                        dma_transfer_ = false;
                        dma_dummy_ = true;
                    }
                }
            }
        }
        else {
            // No DMA happening, the CPU is in control of its own destiny.
            cpu_.clock();
        }
    }

    // indicate if output audio sample is ready
    // do synchronization!!!
    bool is_audio_sample_ready = false;
    audio_time_ += audio_time_in_nes_clock_;
    if (audio_time_ >= audio_time_in_sys_sample_) {
        // reset audio time accumulator
        audio_time_ -= audio_time_in_sys_sample_;
        audio_sample_ = apu_.getOutputSample();
        is_audio_sample_ready = true;
    }

    // The PPU is capable of emitting an interrupt to indicate the
    // vertical blanking period has been entered.
    if (ppu_.nmi) {
        ppu_.nmi = false;
        cpu_.nmi();
    }

    sys_clock_counter_ += 1;

    return is_audio_sample_ready;
}

void Bus::setAudioSampleFrequency(uint32_t sample_rate)
{
    audio_time_in_sys_sample_ = 1.0 / static_cast<double>(sample_rate);
    // NES Dev wiki - NTSC video: https://www.nesdev.org/wiki/NTSC_video
    //
    // The NTSC master clock is 21.47727273 MHz and each PPU pixel lasts
    // four of these clocks: 186ns.
    audio_time_in_nes_clock_ = 1.0 / 5369318.1825; // PPU Clock Frequency
}

} // namespace tn