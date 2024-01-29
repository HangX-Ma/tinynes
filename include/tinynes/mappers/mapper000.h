#ifndef TINYNES_MAPPERS_MAPPER000_H
#define TINYNES_MAPPERS_MAPPER000_H

#include "tinynes/mapper_base.h"
namespace tn
{

class Mapper000 : public MapperBase
{
public:
    Mapper000(uint8_t prg_banks, uint8_t chr_banks) : MapperBase(prg_banks, chr_banks){};
    virtual ~Mapper000() = default;
    // NES Dev wiki - NROM: <https://www.nesdev.org/wiki/NROM>
    //
    // PRG ROM size: 16 KiB for NROM-128, 32 KiB for NROM-256 (DIP-28 standard pinout)
    // PRG RAM: 2 or 4 KiB, not bankswitched, only in Family Basic (but most emulators provide 8)
    // CHR capacity: 8 KiB ROM (DIP-28 standard pinout) but most emulators support RAM
    //
    //  All Banks are fixed,
    //
    //  - CPU $6000-$7FFF: Family Basic only: PRG RAM, mirrored as necessary to fill entire 8 KiB
    //                     window, write protectable with an external switch.
    //  - CPU $8000-$BFFF: First 16 KB of ROM.
    //  - CPU $C000-$FFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
    //
    //  No mapping for PPU!!!

    bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) override;
    bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
    void reset() override;
};

} // namespace tn

#endif