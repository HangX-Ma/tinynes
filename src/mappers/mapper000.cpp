#include "tinynes/mappers/mapper000.h"

namespace tn
{

bool Mapper000::cpuMapRead(uint16_t addr, uint32_t &mapped_addr)
{
    // The generic designation NROM has two PRG ROM capability version, 16K or 32K.
    //      [CPU Address]   [Type]      [PRG ROM]
    // PRGROM is 16K:
    //      $8000-$BFFF     Map         $0000-$3FFF
    //      $C000-$FFFF     Mirror      $8000-$BFFF
    // PRGROM is 32K:
    //      $8000-$FFFF     Map         $0000-$7FFF

    if (addr >= 0x8000 && addr <= 0xFFFF) {
        mapped_addr = addr & (prg_banks_num > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper000::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr)
{
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        mapped_addr = addr & (prg_banks_num > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }

    return false;
}

bool Mapper000::ppuMapRead(uint16_t addr, uint32_t &mapped_addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        mapped_addr = addr;
        return true;
    }

    return false;
}

bool Mapper000::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // treat it as RAM if no CHR banks exist
        if (chr_banks_num == 0) {
            mapped_addr = addr;
            return true;
        }
    }

    return false;
}

} // namespace tn