#ifndef TINYNES_PPU_H
#define TINYNES_PPU_H

#include "tinynes/cartridge.h"
#include <cstdint>
#include <memory>

namespace tn
{

class Cartridge;
class VScreen;

/**
 * PPU also has its own memory map, NES Dev wiki provides
 * <https://www.nesdev.org/wiki/PPU_memory_map> for reference.
 * In this project, we simulate 2c02 PPU work flow.
 */
class PPU
{
public:
    PPU();

    uint8_t cpuRead(uint16_t addr, bool read_only = false);
    void cpuWrite(uint16_t addr, uint8_t data);

    uint8_t ppuRead(uint16_t addr, bool read_only = false);
    void ppuWrite(uint16_t addr, uint8_t data);

    auto vScreenMain() { return vscreen_main_; }
    auto vScreenNameTable(uint8_t idx) { return vscreen_name_table_[idx]; }
    auto vScreenPatternTable(uint8_t idx) { return vscreen_pattern_table_[idx]; }

    bool getFrameState() { return frame_complete_; }
    void setFrameState(bool status) { frame_complete_ = status; }

public:
    // PPU system interfaces
    void connectCartridge(const std::shared_ptr<Cartridge> &cartridge);
    void clock();

private:
    std::shared_ptr<Cartridge> cart_;
    bool frame_complete_{false};
    uint32_t scan_line_{0};
    uint32_t cycle_{0};

    // <https://www.nesdev.org/wiki/PPU_registers#PPUCTRL>
    [[maybe_unused]] union PPUCTRL // $2000, write
    {
        struct
        {
            uint8_t name_table_x : 1;
            uint8_t name_table_y : 1;
            uint8_t vram_addr_mode : 1; // VRAM address increment per CPU read/write of PPUDATA
                                        // (0: add 1, going across; 1: add 32, going down)
            uint8_t sprite_pattern_table_addr : 1;     // (0: $0000; 1: $1000; ignored in 8x16 mode)
            uint8_t background_pattern_table_addr : 1; // (0: $0000; 1: $1000)
            uint8_t sprite_size : 1;                   // (0: 8x8 pixels; 1: 8x16 pixels
            uint8_t slave_mode : 1; // (0: read backdrop from EXT pins; 1: output color on EXT pins)
            uint8_t enable_nmi : 1; // NMI at the start of vertical blanking interval(0: off 1: on)
        };
        uint8_t reg;
    } control_;

    [[maybe_unused]] union PPUMASK // $2001, write
    {
        struct
        {
            uint8_t grayscale : 1;              // (0: normal color, 1: produce a grayscale display)
            uint8_t render_background_left : 1; // (0: off, 1: on)
            uint8_t render_sprites_left : 1;
            uint8_t render_background : 1;
            uint8_t render_sprites : 1;
            uint8_t enhance_red : 1;
            uint8_t enhance_green : 1;
            uint8_t enhance_blue : 1;
        };
        uint8_t reg;

    } mask_;

    [[maybe_unused]] union PPUSTATUS // $2002, read
    {
        struct
        {
            uint8_t unused : 5;
            uint8_t sprite_overflow : 1;
            uint8_t sprite_zero_hit : 1;
            uint8_t vertical_blank : 1;
        };
        uint8_t reg;
    } status_;

private:
    std::shared_ptr<VScreen> vscreen_main_{nullptr};
    std::shared_ptr<VScreen> vscreen_name_table_[2]{nullptr, nullptr};
    std::shared_ptr<VScreen> vscreen_pattern_table_[2]{nullptr, nullptr};

    /**
     * The NES has four logical nametables, but the NES system board itself has only 2 KiB of VRAM,
     * enough for two physical nametables; hardware on the cartridge controls address bit 10 of
     * CIRAM to map one nametable on top of another.
     *
     * @ref NES Dev wiki - PPU nametables: https://www.nesdev.org/wiki/PPU_nametables
     */
    [[maybe_unused]] uint8_t name_table_[2][1024];

    /**
     * The pattern table is divided into two 256-tile sections: $0000-$0FFF, nicknamed "left", and
     * $1000-$1FFF, nicknamed "right".
     * @verbatim PPU addresses within the pattern tables can be decoded as follows:
     *  DCBA98 76543210
     *  ---------------
     *  0HNNNN NNNNPyyy
     *  |||||| |||||+++- T : Fine Y offset, the row number within a tile
     *  |||||| ||||+---- P : Bit plane (0: less significant bit; 1: more significant bit)
     *  ||++++-++++----- N : Tile number from name table
     *  |+-------------- H : Half of pattern table (0: "left"; 1: "right")
     *  +--------------- 0 : Pattern table is at $0000-$1FFF
     * @verbatim
     * @ref NES Dev wiki - PPU pattern tables: <https://www.nesdev.org/wiki/PPU_pattern_tables>
     */
    [[maybe_unused]] uint8_t pattern_table_[2][4096];
};

} // namespace tn

#endif