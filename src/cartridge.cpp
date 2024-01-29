#include "tinynes/cartridge.h"
#include "tinynes/mappers/mapper000.h"

#include <fstream>
#include <spdlog/spdlog.h>

namespace tn
{

Cartridge::Cartridge(std::string_view filename)
{
    std::ifstream ifs;

    ifs.open(filename.data(), std::ifstream::binary);
    if (ifs.is_open()) {
        // read file header
        ifs.read(reinterpret_cast<char *>(&header_), sizeof(INESHeader));

        // escape trainer to acquire PRG ROM data and CHR ROM data
        if ((header_.mapper1 & (1 << 2)) != 0) {
            ifs.seekg(512, std::ifstream::cur);
        }

        // assemble mapper id
        mapper_id_ = (header_.mapper2 & 0xF0) | (header_.mapper1 >> 4);
        mirror = (header_.mapper1 & 0x01) != 0 ? VERTICAL : HORIZONTAL;

        INESFileFormat format = INESFileFormat::iNES1d0;

        switch (format) {
        case INESFileFormat::iNES1d0:
        {
            prg_banks_num_ = header_.prg_rom_size;
            prg_mem_.resize(prg_banks_num_ * 16 * 1024);
            ifs.read(reinterpret_cast<char *>(prg_mem_.data()), prg_mem_.size());

            chr_banks_num_ = header_.chr_rom_size;
            // allocate RAM for CHR memory
            if (chr_banks_num_ == 0) {
                chr_mem_.resize(8 * 1024);
            }
            // allocate space for CHR ROM
            else {
                chr_mem_.resize(chr_banks_num_ * 8 * 1024);
            }
            ifs.read(reinterpret_cast<char *>(chr_mem_.data()), chr_mem_.size());
            break;
        }
        // <https://www.nesdev.org/wiki/NES_2.0>
        case INESFileFormat::NES2d0:
        {
            break;
        }
        }

        switch (mapper_id_) {
        case 0:
            spdlog::info("Cartridge load mapper000");
            mapper_ = std::make_shared<Mapper000>(prg_banks_num_, chr_banks_num_);
            break;
        }
        is_file_loaded_ = true;
        ifs.close();
    }
}

bool Cartridge::cpuRead(uint16_t addr, uint8_t &data)
{
    uint32_t mapped_addr = 0;
    if (mapper_->cpuMapRead(addr, mapped_addr)) {
        data = prg_mem_[mapped_addr];
        return true;
    }
    return false;
}
bool Cartridge::cpuWrite(uint16_t addr, uint8_t data)
{
    uint32_t mapped_addr = 0;
    if (mapper_->cpuMapWrite(addr, mapped_addr, data)) {
        prg_mem_[mapped_addr] = data;
        return true;
    }
    return false;
}

bool Cartridge::ppuRead(uint16_t addr, uint8_t &data)
{
    uint32_t mapped_addr = 0;
    if (mapper_->ppuMapRead(addr, mapped_addr)) {
        data = chr_mem_[mapped_addr];
        return true;
    }
    return false;
}
bool Cartridge::ppuWrite(uint16_t addr, uint8_t data)
{
    uint32_t mapped_addr = 0;
    if (mapper_->ppuMapRead(addr, mapped_addr)) {
        chr_mem_[mapped_addr] = data;
        return true;
    }
    return false;
}

void Cartridge::reset()
{
    // Note: This does not reset the ROM contents,
    // but does reset the mapper.
    if (mapper_ != nullptr) {
        mapper_->reset();
    }
}

} // namespace tn