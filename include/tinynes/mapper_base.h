#ifndef TINYNES_MAPPER_BASE_H
#define TINYNES_MAPPER_BASE_H

#include <cstdint>

namespace tn
{

class MapperBase
{
public:
    MapperBase(uint8_t prg_banks, uint8_t chr_banks)
        : prg_banks_num(prg_banks), chr_banks_num(chr_banks)
    {
        reset();
    };

    // Transform CPU bus address into PRG ROM offset
    virtual bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr) = 0;

    // Transform PPU bus address into CHR ROM offset
    virtual bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) = 0;

    virtual void reset() {}

protected:
    uint8_t prg_banks_num{0};
    uint8_t chr_banks_num{0};
};
} // namespace tn

#endif