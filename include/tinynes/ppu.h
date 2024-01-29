#ifndef TINYNES_PPU_H
#define TINYNES_PPU_H

#include <SFML/Graphics/Color.hpp>
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
    std::shared_ptr<VScreen> vScreenPatternTable(uint8_t idx, uint8_t palette);
    auto oam() { return oam_ptr_; }

    bool getFrameState() { return frame_complete_; }
    void setFrameState(bool status) { frame_complete_ = status; }

public:
    // PPU system interfaces
    void connectCartridge(const std::shared_ptr<Cartridge> &cartridge);
    void reset();
    void clock();
    bool nmi{false};

private:
    sf::Color getColorFromPaletteMemory(uint8_t palette, uint8_t pixel);

private:
    std::shared_ptr<Cartridge> cart_;
    bool frame_complete_{false};
    int32_t scanline_{0};
    int32_t cycle_{0};

    // <https://www.nesdev.org/wiki/PPU_registers#PPUCTRL>
    union PPUCTRL // $2000, write
    {
        struct
        {
            uint8_t name_table_x : 1;
            uint8_t name_table_y : 1;
            uint8_t vram_addr_mode : 1; // VRAM address increment per CPU read/write of PPUDATA
                                        // (0: add 1, going across; 1: add 32, going down)
            uint8_t sprite_pattern_table_addr : 1; // (0: $0000; 1: $1000; ignored in 8x16 mode)
            uint8_t
                background_pattern_table_addr : 1; // (0: $0000, left half; 1: $1000, right half)
            uint8_t sprite_size : 1;               // (0: 8x8 pixels; 1: 8x16 pixels
            uint8_t slave_mode : 1; // (0: read backdrop from EXT pins; 1: output color on EXT pins)
            uint8_t enable_nmi : 1; // NMI at the start of vertical blanking interval(0: off 1: on)
        };
        uint8_t reg;
    } control_;

    union PPUMASK // $2001, write
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

    union PPUSTATUS // $2002, read
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

    // NES Dev wiki - PPU scrolling: https://www.nesdev.org/wiki/PPU_scrolling
    //
    // PPU internal registers
    // v: Current VRAM address (15 bits)
    // t: Temporary VRAM address (15 bits); can also be thought of as the address of the top left
    //    onscreen tile.
    // x: Fine X scroll (3 bits)
    // w: First or second write toggle (1 bit)
    //
    // The 15 bit registers t and v are composed this way during rendering:
    //
    // yyy NN YYYYY XXXXX
    // ||| || ||||| +++++-- coarse X scroll
    // ||| || +++++-------- coarse Y scroll
    // ||| ++-------------- nametable select
    // +++----------------- fine Y scroll
    //
    // Note that while the v register has 15 bits, the PPU memory space is only 14 bits wide. The
    // highest bit is unused for access through $2007.
    union LoopyRegister
    {
        struct
        {
            // coarse x/y select which tile
            uint16_t coarse_x : 5;
            uint16_t coarse_y : 5;
            // nametable x/y select one specific nametable area
            uint16_t nametable_x : 1;
            uint16_t nametable_y : 1;
            // fine x/y select the specific pixel in 8x8 tile
            uint16_t fine_y : 3;
            uint16_t unused : 1;
        };
        uint16_t reg{0x0000};
    };

    // PPU internal registers
    // NES Dev wiki - PPU scrolling, Register controls: <https://www.nesdev.org/wiki/PPU_scrolling>
    // tells how to use the v,t,x,w registers
    LoopyRegister vram_addr_;
    LoopyRegister tram_addr_;
    uint8_t fine_x_{0};

    // assistant variables used in PPU registers access
    /**
     * PPUADDR
     * Address ($2006) >> write x2
     *
     * The CPU writes to VRAM through a pair of registers on the PPU by first loading an address
     * into PPUADDR and then it writing data repeatedly to PPUDATA. The 16-bit address is written to
     * PPUADDR one byte at a time, upper byte first. Whether this is the first or second write is
     * tracked internally by the w register, which is shared with PPUSCROLL.
     *
     * After reading PPUSTATUS to clear w (the write latch), write the 16-bit address of VRAM you
     * want to access here, upper byte first.
     */
    uint8_t address_latch_{0};
    /**
     * PPUDATA
     * Data ($2007) <> read/write
     *
     * Note that the internal read buffer is updated only on PPUDATA reads. It is not affected by
     * other PPU processes such as rendering, and it maintains its value indefinitely until the next
     * read.
     */
    uint8_t data_buffer_{0x00};

    // Background rendering
    struct BGNextTile
    {
        uint8_t id{0x00};
        uint8_t attribute{0x00};
        uint8_t lsb{0x00};
        uint8_t msb{0x00};
    } bg_next_tile_;

    struct BGShifter
    {
        uint16_t lo{0x0000};
        uint16_t hi{0x0000};
    };
    BGShifter bg_shifter_pattern_;
    BGShifter bg_shifter_attribute_;

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
    uint8_t name_table_[2][1024];

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
    uint8_t pattern_table_[2][4096];

    /* palette colors */
    uint8_t palette_table_[32];

    // https://www.nesdev.org/wiki/PPU_OAM
    // Byte 0: Y position of top of sprite
    // Byte 1: Tile index number
    //   - For 8x8 sprites, this is the tile number of this sprite within the pattern table selected
    //   in bit 3 of PPUCTRL ($2000).
    //   - For 8x16 sprites (bit 5 of PPUCTRL set), the PPU ignores the pattern table selection and
    //   selects a pattern table from bit 0 of this number.
    //
    //   76543210
    //   ||||||||
    //   |||||||+- Bank ($0000 or $1000) of tiles
    //   +++++++-- Tile number of top of sprite (0 to 254; bottom half gets the next tile)
    // Byte 2: Attribute
    //
    //   76543210
    //   ||||||||
    //   ||||||++- Palette (4 to 7) of sprite
    //   |||+++--- Unimplemented (read 0)
    //   ||+------ Priority (0: in front of background; 1: behind background)
    //   |+------- Flip sprite horizontally
    //   +-------- Flip sprite vertically
    // Byte 3: X position of left side of sprite.
    struct ObjectAttributeEntry
    {
        uint8_t y;         // Y position of sprite
        uint8_t id;        // ID of tile from pattern memory
        uint8_t attribute; // Flags define how sprite should be rendered
        uint8_t x;         // X position of sprite
    } OAM_[64];
    uint8_t *oam_ptr_ = reinterpret_cast<uint8_t *>(OAM_);

    // A register to store the address when the CPU manually communicates
    // with OAM via PPU registers. This is not commonly used because it
    // is very slow, and instead a 256-Byte DMA transfer is used. See
    // the Bus header for a description of this.
    uint8_t oam_addr_{0x00};

    // NES Dev wiki - Sprite overflow games: <https://www.nesdev.org/wiki/Sprite_overflow_games>
    //
    // The sprite overflow flag is rarely used, mainly due to bugs when exactly 8 sprites are
    // present on a scanline. No games rely on the buggy behavior.
    //
    // Nonetheless, games can intentionally place 9 or more sprites in a scanline to trigger the
    // overflow flag consistently, as long as no previous scanlines have exactly 8 sprites.
    ObjectAttributeEntry sprite_per_scanline_[8];
    uint8_t sprite_count_;
    uint8_t sprite_shifter_pattern_lo_[8];
    uint8_t sprite_shifter_pattern_hi_[8];

    // NES Dev wiki - PPU OAM: < https : // www.nesdev.org/wiki/PPU_OAM#Sprite_0_hits>
    //  Sprite Zero Collision Flags
    bool sprite_zero_hit_possible_ = false;
    bool sprite_zero_being_rendered_ = false;

    // The OAM is conveniently package above to work with, but the DMA
    // mechanism will need access to it for writing one byte at a time
};

} // namespace tn

#endif