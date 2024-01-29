#include "tinynes/ppu.h"
#include "tinynes/cartridge.h"
#include "tinynes/palette_color.h"
#include "tinynes/vscreen.h"

#include <memory>

namespace tn
{

PPU::PPU()
{
    vscreen_main_ = std::make_shared<VScreen>(256, 240, sf::Color::Black);
    vscreen_name_table_[0] = std::make_shared<VScreen>(256, 240, sf::Color::Black);
    vscreen_name_table_[1] = std::make_shared<VScreen>(256, 240, sf::Color::Black);
    vscreen_pattern_table_[0] = std::make_shared<VScreen>(128, 128, sf::Color::Black);
    vscreen_pattern_table_[1] = std::make_shared<VScreen>(128, 128, sf::Color::Black);
}

/**
 * Background palette ranges from $3F00 to $3F0F, according to palette composition rules, $3F00 to
 * $3F0F memory space is divided into 4 parts.
 *
 * |  Address | |     Purpose      |
 * $3F01-$3F03  Background palette 0
 * $3F05-$3F07  Background palette 1
 * $3F09-$3F0B  Background palette 2
 * $3F0D-$3F0F  Background palette 3
 *
 * @ref NES Dev wiki - PPU palettes: <https://www.nesdev.org/wiki/PPU_palettes>
 */
sf::Color PPU::getColorFromPaletteMemory(uint8_t palette, uint8_t pixel)
{
    return sf::Color(COLORS[ppuRead(0x3F00 + (palette << 2) + pixel) & 0x3F]);
}

/**
 * According to NES Dev wiki description, PPU has a pattern table to define the shapes of tiles that
 * makes update the backgrounds and sprites. Generally, each tile in pattern table is 16 bytes, made
 * of two planes. So we can individually divide them to LSB plane and MSB plane, where the tiles in
 * each plane are 8 bytes size, occupying 8x8 pixels.
 *
 * LSB and MSB tile combines and provide 4 types of pixel functions:
 * - 00: transparent color
 * - 01: color index 1
 * - 10: color index 2
 * - 11: color index 3
 *
 * The planes are stored as 8 bytes of LSB, followed by 8 bytes of MSB.
 *
 * According to [PPU memory map]<https://www.nesdev.org/wiki/PPU_memory_map>, the pattern table is
 * divided into two 256-tile sections: $0000-$0FFF, nicknamed "left", and $1000-$1FFF, nicknamed
 * "right".
 *
 * @param idx pattern table index, 0 'left', 1 'right'
 * @ref NES Dev wiki - PPU pattern tables: <https://www.nesdev.org/wiki/PPU_pattern_tables>
 * @warning Don't forget to call update() function after you require the updated pattern table
 *          sprite.
 * @return Vscreen smart shared pointer used for drawing
 */
std::shared_ptr<VScreen> PPU::vScreenPatternTable(uint8_t idx, uint8_t palette)
{
    for (uint16_t ytile = 0; ytile < 16; ytile += 1) {
        for (uint16_t xtile = 0; xtile < 16; xtile += 1) {
            // each tile is 16 bytes !!!
            uint16_t offset = ytile * 256 + xtile * 16;

            // now loop through the 8x8 pixels  tile
            for (uint16_t row = 0; row < 8; row += 1) {
                // If you get confused with the following code, please look at "Bit Planes" example
                // on <https://www.nesdev.org/wiki/PPU_pattern_tables>. 'ppuRead' read 1 byte each
                // time.
                uint8_t tile_lsb = ppuRead(idx * 0x1000 + offset + row + 0x0000);
                uint8_t tile_msb = ppuRead(idx * 0x1000 + offset + row + 0x0008);

                for (uint16_t col = 0; col < 8; col += 1) {
                    uint8_t pixel = ((tile_lsb & 0x01) << 1) | (tile_msb & 0x01);
                    tile_lsb >>= 1;
                    tile_msb >>= 1;

                    vscreen_pattern_table_[idx]->setPixel(
                        xtile * 8 + (7 - col), // inverse to draw pixels from left
                        ytile * 8 + row, getColorFromPaletteMemory(palette, pixel));
                }
            }
        }
    }
    return vscreen_pattern_table_[idx];
}

uint8_t PPU::cpuRead(uint16_t addr, [[maybe_unused]] bool read_only)
{
    uint8_t data = 0x00;

    // 'read_only' will protect the PPU register stay unchanged when it is read
    if (read_only) {
        switch (addr) {
        case 0x0000: // PPUCTRL
            data = control_.reg;
            break;
        case 0x0001: // PPUMASK
            data = mask_.reg;
            break;
        case 0x0002: // PPUSTATUS
            data = status_.reg;
            break;
        case 0x0003: // OAMADDR
            break;
        case 0x0004: // OAMDATA
            break;
        case 0x0005: // PPUSCROLL
            break;
        case 0x0006: // PPUADDR
            break;
        case 0x0007: // PPUDATA
            break;
        default:
            break;
        }
    }
    else {
        switch (addr) {
        case 0x0000: // PPUCTRL - Not readable
            break;
        case 0x0001: // PPUMASK - Not readable
            break;
        case 0x0002: // PPUSTATUS
            // PPUSTATUS has upper 3 bits flag and lower 5 bits stale PPU bus contents.
            data = (status_.reg & 0xE0) | (data_buffer_ & 0x1F);
            // clear 'V' bit after reading
            status_.vertical_blank = 0;
            // After reading PPUSTATUS to clear w (the write latch)
            address_latch_ = 0;
            break;
        case 0x0003: // OAMADDR - Unused
            break;
        case 0x0004: // OAMDATA - Unused
            break;
        case 0x0005: // PPUSCROLL - Not readable
            break;
        case 0x0006: // PPUADDR - Not readable
            break;
        case 0x0007: // PPUDATA
            // Reads from the NameTable ram get delayed one cycle,
            // so output buffer which contains the data from the
            // previous read request. Reading upper byte first!!!
            data = data_buffer_;

            // prepare for next time reading
            data_buffer_ = ppuRead(vram_addr_.reg);

            // If the address was in the palette range, the
            // data is not delayed, so it returns immediately
            if (vram_addr_.reg >= 0x3F00) {
                data = data_buffer_;
            }
            vram_addr_.reg += (control_.vram_addr_mode ? 32 /*down*/ : 1 /*across*/);
            break;
        default:
            break;
        }
    }

    return data;
}
void PPU::cpuWrite(uint16_t addr, uint8_t data)
{
    switch (addr) {
    case 0x0000: // PPUCTRL
        control_.reg = data;
        tram_addr_.nametable_x = control_.name_table_x;
        tram_addr_.nametable_y = control_.name_table_y;
        break;
    case 0x0001: // PPUMASK
        mask_.reg = data;
        break;
    case 0x0002: // PPUSTATUS
        break;
    case 0x0003: // OAMADDR
        break;
    case 0x0004: // OAMDATA
        break;
    // <https://www.nesdev.org/wiki/PPU_registers#PPUSCROLL>
    // PPUSCROLL takes two writes: the first is the X scroll and the second is the Y scroll. Whether
    // this is the first or second write is tracked internally by the w register, which is shared
    // with PPUADDR.
    //
    // The high 5 bits indicate the coarse X/Y
    // The low 3 bits indicate the fine X/Y
    case 0x0005: // PPUSCROLL
        if (address_latch_ == 0) {
            fine_x_ = data & 0x07;
            tram_addr_.coarse_x = data >> 3;
            address_latch_ = 1;
        }
        else {
            tram_addr_.fine_y = data & 0x07;
            tram_addr_.coarse_y = data >> 3;
            address_latch_ = 0;
        }
        break;
    // The 16-bit address is written to PPUADDR one byte at a time, upper byte first. Whether this
    // is the first or second write is tracked internally by the w register, which is shared with
    // PPUSCROLL. Writing to PPUADDR register is limited in blanking area.
    case 0x0006: // PPUADDR
        if (address_latch_ == 0) {
            // If you have seen the 'Register Control' part in
            // <https://www.nesdev.org/wiki/PPU_scrolling#$2006_first_write_(w_is_0)>,
            // you will know the upper 2 bits isn't used so we use (data & 0x3F)
            tram_addr_.reg
                = ((static_cast<uint16_t>(data) & 0x3F) << 8) | (tram_addr_.reg & 0x00FF);
            address_latch_ = 1;
        }
        else {
            tram_addr_.reg = (tram_addr_.reg & 0xFF00) | static_cast<uint16_t>(data);
            vram_addr_.reg = tram_addr_.reg;
            address_latch_ = 0;
        }
        break;
    case 0x0007: // PPUDATA
        ppuWrite(vram_addr_.reg, data);
        // All writes from PPU data automatically increment the nametable
        // address depending upon the mode set in the control register.
        vram_addr_.reg += (control_.vram_addr_mode ? 32 : 1);
        break;
    default:
        break;
    }
}

uint8_t PPU::ppuRead(uint16_t addr, [[maybe_unused]] bool read_only)
{
    uint8_t data = 0x00;
    addr &= 0x3FFF;

    if (cart_->ppuRead(addr, data)) {
    }
    // 2 pattern tables, per size 0x1000
    else if (addr >= 0x0000 && addr <= 0x1FFF) {
        int table_index = (addr & 0x1000) >> 12;
        data = pattern_table_[table_index][addr & 0x0FFF];
    }
    // 4 name tables, per size 0x400
    // NES Dev wiki - PPU nametables: <https://www.nesdev.org/wiki/PPU_nametables>
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        addr &= 0x0FFF;
        // Vertical
        // $2000 equals $2800 and $2400 equals $2C00
        if (cart_->mirror == Cartridge::MIRROR::VERTICAL) {
            if (addr >= 0x0000 && addr <= 0x03FF) {
                data = name_table_[0][addr & 0x03FF];
            }
            if (addr >= 0x0400 && addr <= 0x07FF) {
                data = name_table_[1][addr & 0x03FF];
            }
            if (addr >= 0x0800 && addr <= 0x0BFF) {
                data = name_table_[0][addr & 0x03FF];
            }
            if (addr >= 0x0C00 && addr <= 0x0FFF) {
                data = name_table_[1][addr & 0x03FF];
            }
        }
        // Horizontal
        // $2000 equals $2400 and $2800 equals $2C00
        else if (cart_->mirror == Cartridge::MIRROR::HORIZONTAL) {
            if (addr >= 0x0000 && addr <= 0x03FF) {
                data = name_table_[0][addr & 0x03FF];
            }
            if (addr >= 0x0400 && addr <= 0x07FF) {
                data = name_table_[0][addr & 0x03FF];
            }
            if (addr >= 0x0800 && addr <= 0x0BFF) {
                data = name_table_[1][addr & 0x03FF];
            }
            if (addr >= 0x0C00 && addr <= 0x0FFF) {
                data = name_table_[1][addr & 0x03FF];
            }
        }
    }
    // palette RAM indexes
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        // we only care about the lower 5 bits that relates to background or foreground colors
        addr &= 0x001F;
        // $3F10 is the mirror of universal background color stored in $3F00
        if (addr == 0x0010) {
            addr = 0x0000;
        }
        // What's more, we also need to deal with the mirrored palette colors
        if (addr == 0x0014) {
            addr = 0x0004;
        }
        if (addr == 0x0018) {
            addr = 0x0008;
        }
        if (addr == 0x001C) {
            addr = 0x000C;
        }
        data = palette_table_[addr] & (mask_.grayscale ? 0x30 : 0x3F);
    }

    return data;
}

void PPU::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;

    if (cart_->ppuWrite(addr, data)) {
    }
    // 2 pattern tables, per size 0x1000
    else if (addr >= 0x0000 && addr <= 0x1FFF) {
        int table_index = (addr & 0x1000) >> 12;
        pattern_table_[table_index][addr & 0x0FFF] = data;
    }
    // 4 name tables, per size 0x400
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        addr &= 0x0FFF;
        if (cart_->mirror == Cartridge::MIRROR::VERTICAL) {
            // Vertical
            if (addr >= 0x0000 && addr <= 0x03FF) {
                name_table_[0][addr & 0x03FF] = data;
            }
            if (addr >= 0x0400 && addr <= 0x07FF) {
                name_table_[1][addr & 0x03FF] = data;
            }
            if (addr >= 0x0800 && addr <= 0x0BFF) {
                name_table_[0][addr & 0x03FF] = data;
            }
            if (addr >= 0x0C00 && addr <= 0x0FFF) {
                name_table_[1][addr & 0x03FF] = data;
            }
        }
        else if (cart_->mirror == Cartridge::MIRROR::HORIZONTAL) {
            // Horizontal
            if (addr >= 0x0000 && addr <= 0x03FF) {
                name_table_[0][addr & 0x03FF] = data;
            }
            if (addr >= 0x0400 && addr <= 0x07FF) {
                name_table_[0][addr & 0x03FF] = data;
            }
            if (addr >= 0x0800 && addr <= 0x0BFF) {
                name_table_[1][addr & 0x03FF] = data;
            }
            if (addr >= 0x0C00 && addr <= 0x0FFF) {
                name_table_[1][addr & 0x03FF] = data;
            }
        }
    }
    // palette RAM indexes
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F;
        if (addr == 0x0010) {
            addr = 0x0000;
        }
        if (addr == 0x0014) {
            addr = 0x0004;
        }
        if (addr == 0x0018) {
            addr = 0x0008;
        }
        if (addr == 0x001C) {
            addr = 0x000C;
        }
        palette_table_[addr] = data;
    }
}

void PPU::connectCartridge(const std::shared_ptr<Cartridge> &cartridge) { cart_ = cartridge; }

void PPU::reset()
{
    fine_x_ = 0x00;
    address_latch_ = 0x00;
    data_buffer_ = 0x00;
    scan_line_ = 0;
    cycle_ = 0;
    status_.reg = 0x00;
    mask_.reg = 0x00;
    control_.reg = 0x00;
    vram_addr_.reg = 0x0000;
    tram_addr_.reg = 0x0000;
    bg_next_tile_.id = 0x00;
    bg_next_tile_.attribute = 0x00;
    bg_next_tile_.lsb = 0x00;
    bg_next_tile_.msb = 0x00;
    shifter_pattern_.lo = 0x0000;
    shifter_pattern_.hi = 0x0000;
    shifter_attribute_.lo = 0x0000;
    shifter_attribute_.hi = 0x0000;
}

void PPU::clock()
{
    std::function<void()> transfer_address_x_func = [&]()
    {
        // Ony if rendering is enabled
        if (mask_.render_background || mask_.render_sprites) {
            vram_addr_.nametable_x = tram_addr_.nametable_x;
            vram_addr_.coarse_x = tram_addr_.coarse_x;
        }
    };

    std::function<void()> transfer_address_y_func = [&]()
    {
        // Ony if rendering is enabled
        if (mask_.render_background || mask_.render_sprites) {
            vram_addr_.fine_y = tram_addr_.fine_y;
            vram_addr_.nametable_y = tram_addr_.nametable_y;
            vram_addr_.coarse_y = tram_addr_.coarse_y;
        }
    };

    std::function<void()> increment_scrolly_func = [&]()
    {
        // NES Dev wiki - PPU scrolling : Coarse Y increment
        // <https://www.nesdev.org/wiki/PPU_scrolling#Y_increment>
        //
        // If rendering is enabled, fine Y is incremented at dot 256 of each scanline, overflowing
        // to coarse Y, and finally adjusted to wrap among the nametables vertically.
        if (mask_.render_background || mask_.render_sprites) {
            if (vram_addr_.fine_y < 7) {
                vram_addr_.fine_y += 1;
            }
            else {
                vram_addr_.fine_y = 0;
                if (vram_addr_.coarse_y == 29) {
                    vram_addr_.coarse_y = 0;
                    // switch vertical nametable
                    vram_addr_.nametable_y = ~vram_addr_.nametable_y;
                }
                // The bottom two rows do not contain tile information
                else if (vram_addr_.coarse_y == 31) {
                    vram_addr_.coarse_y = 0;
                }
                else {
                    vram_addr_.coarse_y += 1;
                }
            }
        }
    };

    std::function<void()> increment_scrollx_func = [&]()
    {
        // NES Dev wiki - PPU scrolling : Coarse X increment
        // <https://www.nesdev.org/wiki/PPU_scrolling#Coarse_X_increment>
        //
        // only when render is enabled
        if (mask_.render_background || mask_.render_sprites) {
            if (vram_addr_.coarse_x == 31) {
                vram_addr_.coarse_x = 0;
                // switch horizontal nametable
                vram_addr_.nametable_x = ~vram_addr_.nametable_x;
            }
            else {
                vram_addr_.coarse_x += 1;
            }
        }
    };

    std::function<void()> load_shifter_func = [&]()
    {
        // Each PPU cycle update we calculate one pixel.
        // Shifter is 16 bits wide, because the top 8 bits are the current 8 pixels being
        // drawn and the bottom 8 bits are the next 8 pixels to be drawn.
        shifter_pattern_.lo = (shifter_pattern_.lo & 0xFF00) | bg_next_tile_.lsb;
        shifter_pattern_.hi = (shifter_pattern_.hi & 0xFF00) | bg_next_tile_.msb;

        // Attribute bits do not change per pixel, rather they change every 8 pixels
        // but are synchronized with the pattern shifters for convenience, so here
        // we take the bottom 2 bits of the attribute word which represent which
        // palette is being used for the current 8 pixels and the next 8 pixels, and
        // "inflate" them to 8 bit words.
        shifter_attribute_.lo = (shifter_attribute_.lo & 0xFF00)
                                | ((bg_next_tile_.attribute & 0b01) != 0 ? 0xFF : 0x00);
        shifter_attribute_.hi = (shifter_attribute_.hi & 0xFF00)
                                | ((bg_next_tile_.attribute & 0b10) != 0 ? 0xFF : 0x00);
    };

    // Every cycle the shifters storing pattern and attribute information shift
    // their contents by 1 bit, because PPU processes 1 pixel per cycle.
    std::function<void()> update_shifter_func = [&]()
    {
        if (mask_.render_background) {
            // Shifting background tile pattern row
            shifter_pattern_.lo <<= 1;
            shifter_pattern_.hi <<= 1;

            // Shifting palette attributes 1
            shifter_attribute_.lo <<= 1;
            shifter_attribute_.hi <<= 1;
        }
    };

    // NES Dev wiki - PPU rendering: https://www.nesdev.org/wiki/PPU_rendering
    // The above link record 'Frame timing diagram' that tells us how to deal with rendering
    // picture.
    //
    // The PPU renders 262 scanlines per frame. Each scanline lasts for 341 PPU clock cycles
    // (113.667 CPU clock cycles; 1 CPU cycle = 3 PPU cycles), with each clock cycle producing one
    // pixel. The line numbers given here correspond to how the internal PPU frame counters count
    // lines.
    //
    // - Pre-render scanline (-1 or 261)
    // - Visible scanlines (0-239)
    // - Cycle 0
    // - Cycles 1-256
    // - Cycles 257-320
    // - Cycles 321-336
    // - Cycles 337-340
    // - Post-render scanline (240)
    // - Vertical blanking lines (241-260)
    //
    // Please check <https://www.nesdev.org/w/images/default/4/4f/Ppu.svg> !!!

    if (scan_line_ >= -1 && scan_line_ < 240) {
        // For odd frames, the cycle at the end of the scanline is skipped
        if (scan_line_ == 0 && cycle_ == 0) {
            // replacing the idle tick at the beginning of the first visible scanline with the last
            // tick of the last dummy nametable fetch
            cycle_ = 1;
        }
        // For even frames, the last cycle occurs normally.
        if (scan_line_ == -1 && cycle_ == 1) {
            // start the new frame by clearing vertical blank flag
            status_.vertical_blank = 0;
        }

        // tile fetch
        if ((cycle_ >= 2 && cycle_ < 258) || (cycle_ >= 321 && cycle_ < 338)) {
            update_shifter_func();

            // each event timing sequence takes  2 clock cycles
            switch ((cycle_ - 1) % 8) {
            case 0:
            {
                load_shifter_func();
                bg_next_tile_.id = ppuRead(0x2000 | (vram_addr_.reg & 0x0FFF));
                break;
            }
            case 2:
            {
                // Fetch the next background tile attribute.
                //
                // The low 12 bits of the attribute address are composed in the following way:
                //
                // NN 1111 YYY XXX
                // || |||| ||| +++-- high 3 bits of coarse X (x/4)
                // || |||| +++------ high 3 bits of coarse Y (y/4)
                // || ++++---------- attribute offset (960 bytes)
                // ++--------------- nametable select
                //
                // composite address: YX11 11yy yxxx, 12 bits
                //
                // All attribute memory begins at 0x03C0 within a nametable, so OR with
                // result to select target nametable, and attribute byte offset. Finally
                // OR with 0x2000 to offset into nametable address space on PPU bus.
                bg_next_tile_.attribute = ppuRead(
                    0x23C0 | (vram_addr_.nametable_y << 11) | (vram_addr_.nametable_x << 10)
                    | ((vram_addr_.coarse_y >> 2) << 3) | (vram_addr_.coarse_x >> 2));

                // Right we've read the correct attribute byte for a specified address,
                // but the byte itself is broken down further into the 2x2 tile groups
                // in the 4x4 attribute zone.
                //
                // The attribute byte is assembled thus: BR(76) BL(54) TR(32) TL(10)
                // Note: the number in parentheses indicates the bits location
                //
                // +----+----+              +----+----+
                // | TL | TR |              | ID | ID |
                // +----+----+ where TL =   +----+----+
                // | BL | BR |              | ID | ID |
                // +----+----+              +----+----+
                //
                // The following process can simplify the attribute table:
                // We know the lower 2 bits of coarse X/Y controls the mapping place of 2x2 tile
                // groups:
                // - coarse Y(0b1x) means bottom half and coarse Y(0b0x) meas top half
                // - coarse X(0bx1) means right half and coarse X(0bx0) meas right half
                if ((vram_addr_.coarse_y & 0x02) != 0) {
                    bg_next_tile_.attribute >>= 4;
                }
                if ((vram_addr_.coarse_x & 0x02) != 0) {
                    bg_next_tile_.attribute >>= 2;
                }
                // Finally we only use the last two LSB
                bg_next_tile_.attribute &= 0x03;
            }
            case 4:
            {
                // NES Dev wiki - PPU pattern tables:
                // <https://www.nesdev.org/wiki/PPU_pattern_tablesz>
                // PPU addresses within the pattern tables can be decoded as follows:
                //
                // DCBA98 76543210
                // ---------------
                // 0HNNNN NNNNPyyy
                // |||||| |||||+++- T: Fine Y offset, the row number within a tile
                // |||||| ||||+---- P: Bit plane (0: less significant bit; 1: more significant bit)
                // ||++++-++++----- N: Tile number from name table
                // |+-------------- H: Half of pattern table (0: "left"; 1: "right")
                // +--------------- 0: Pattern table is at $0000-$1FFF
                //
                bg_next_tile_.lsb = ppuRead((control_.background_pattern_table_addr << 12)
                                            + (static_cast<uint16_t>(bg_next_tile_.id) << 4)
                                            + (vram_addr_.fine_y) + 0);
                break;
            }
            case 6:
            {
                bg_next_tile_.msb = ppuRead((control_.background_pattern_table_addr << 12)
                                            + (static_cast<uint16_t>(bg_next_tile_.id) << 4)
                                            + (vram_addr_.fine_y) + 8 /*offset to next bit plane*/);
                break;
            }
            case 7:
            {
                // Increment the background tile "pointer" to the next tile horizontally
                // in the nametable memory.
                increment_scrollx_func();
                break;
            }
            }
        }

        if (cycle_ == 256) {
            increment_scrolly_func();
        }

        // reset x position
        if (cycle_ == 257) {
            load_shifter_func();
            transfer_address_x_func();
        }

        // Superfluous reads of tile id at end of scanline
        if (cycle_ == 338 || cycle_ == 340) {
            // NES Dev wiki - Tile and attribute fetching:
            // <https://www.nesdev.org/wiki/PPU_scrolling#Tile_and_attribute_fetching>
            // Note: 0x2000 is the start address of nametable, whose size is 0x1000
            bg_next_tile_.id = ppuRead(0x2000 | (vram_addr_.reg & 0x0FFF));
        }

        // reset y position
        if (scan_line_ == -1 && cycle_ >= 280 && cycle_ < 305) {
            transfer_address_y_func();
        }
    }

    // Post-render scanline
    if (scan_line_ == 240) {
        ; // idle
    }

    // Vertical blanking lines
    if (scan_line_ >= 241 && scan_line_ < 261) {
        if (scan_line_ == 241 && cycle_ == 1) {
            status_.vertical_blank = 1;
            if (control_.enable_nmi) {
                nmi = true;
            }
        }
    }

    // Composition - We now have background pixel information for this cycle
    // At this point we are only interested in background

    uint8_t bg_pixel = 0x00;   // The 2-bit pixel index
    uint8_t bg_palette = 0x00; // The 3-bit index palette index

    if (mask_.render_background) {
        uint16_t bit_mux = 0x8000 >> fine_x_;

        // Select Plane pixels by extracting from the shifter
        // at the required location.
        auto p0_pixel = static_cast<uint8_t>((shifter_pattern_.lo & bit_mux) > 0);
        auto p1_pixel = static_cast<uint8_t>((shifter_pattern_.hi & bit_mux) > 0);

        // Combine to form pixel index
        bg_pixel = (p1_pixel << 1) | p0_pixel;
        // Get palette
        auto bg_pal0 = static_cast<uint8_t>((shifter_attribute_.lo & bit_mux) > 0);
        auto bg_pal1 = static_cast<uint8_t>((shifter_attribute_.hi & bit_mux) > 0);
        bg_palette = (bg_pal1 << 1) | bg_pal0;
    }

    vscreen_main_->setPixel(cycle_ - 1, scan_line_,
                            getColorFromPaletteMemory(bg_palette, bg_pixel));

    // snow noise
    // vscreen_main_->setPixel(cycle_, scan_line_ - 1, sf::Color(COLORS[(rand() % 2) != 0 ? 0x3F :
    // 0x30]));

    // advance rendering
    cycle_ += 1;
    if (cycle_ >= 341) {
        cycle_ = 0;
        scan_line_ += 1;
        if (scan_line_ >= 261) {
            scan_line_ = -1;
            frame_complete_ = true;
        }
    }
}

} // namespace tn