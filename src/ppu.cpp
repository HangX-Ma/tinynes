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

uint8_t PPU::cpuRead(uint16_t addr, [[maybe_unused]] bool read_only)
{
    uint8_t data = 0x00;

    switch (addr) {
    case 0x0000: // PPUCTRL
        break;
    case 0x0001: // PPUMASK
        break;
    case 0x0002: // PPUSTATUS
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
    return data;
}
void PPU::cpuWrite(uint16_t addr, [[maybe_unused]] uint8_t data)
{
    switch (addr) {
    case 0x0000: // PPUCTRL
        break;
    case 0x0001: // PPUMASK
        break;
    case 0x0002: // PPUSTATUS
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

uint8_t PPU::ppuRead(uint16_t addr, [[maybe_unused]] bool read_only)
{
    uint8_t data = 0x00;
    addr &= 0x3FFF;

    if (cart_->ppuRead(addr, data)) {
    }

    return data;
}

void PPU::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;

    if (cart_->ppuWrite(addr, data)) {
    }
}

void PPU::connectCartridge(const std::shared_ptr<Cartridge> &cartridge) { cart_ = cartridge; }

void PPU::clock()
{
    vscreen_main_->setPixel(cycle_, scan_line_, sf::Color(colors[(rand() % 2) != 0 ? 0x3F : 0x30]));
    cycle_ += 1;
    if (cycle_ >= vscreen_main_->width()) {
        cycle_ = 0;
        scan_line_ += 1;
        if (scan_line_ >= vscreen_main_->height()) {
            scan_line_ = -1;
            frame_complete_ = true;
        }
    }
}

} // namespace tn