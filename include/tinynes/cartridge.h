#ifndef TINYNES_CARTRIDGE_H
#define TINYNES_CARTRIDGE_H

#include <memory>
#include <string_view>
#include <vector>
namespace tn
{

class MapperBase;
class Cartridge
{
public:
    explicit Cartridge(std::string_view filename);

    bool cpuRead(uint16_t addr, uint8_t &data);
    bool cpuWrite(uint16_t addr, uint8_t data);

    bool ppuRead(uint16_t addr, uint8_t &data);
    bool ppuWrite(uint16_t addr, uint8_t data);

    bool isNesFileLoaded() { return is_file_loaded_; }

private:
    /**
     * An NES cartridge has at least two memory chips on it: PRG (connected to the CPU) and CHR
     * (connected to the PPU). There is always at least one PRG ROM, and there may be an additional
     * PRG RAM to hold data.
     *
     * The PPU can see only 8 KiB of tile data at once. So once your game has more than that much
     * tile data, you'll need to use a mapper to load more data into the PPU.
     *
     * @ref NES Dev wiki - CHR ROM vs. CHR RAM : <https://www.nesdev.org/wiki/CHR_ROM_vs._CHR_RAM>
     */
    std::vector<uint8_t> prg_mem_;
    std::vector<uint8_t> chr_mem_;

    uint8_t mapper_id_{0};
    uint8_t prg_banks_num_{0};
    uint8_t chr_banks_num_{0};

    std::shared_ptr<MapperBase> mapper_{nullptr};

    bool is_file_loaded_{false};

private:
    // NES Dev wiki - Cartridge iNES file format: https://www.nesdev.org/wiki/INES
    // An iNES file consists of the following sections, in order:
    //
    // 1. Header (16 bytes)
    // 2. Trainer, if present (0 or 512 bytes)
    // 3. PRG ROM data (16384 * x bytes)
    // 4. CHR ROM data, if present (8192 * y bytes)
    // 5. PlayChoice INST-ROM, if present (0 or 8192 bytes)
    // 6. PlayChoice PROM, if present (16 bytes Data, 16 bytes CounterOut)
    struct INESHeader
    {
        uint8_t magic_name[4];
        uint8_t prg_rom_size; // 16 KB units
        uint8_t chr_rom_size; // 8 KB units
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t prg_ram_size;
        uint8_t tv_system1;
        uint8_t tv_system2;
        uint8_t unused[5];
    } header_; // 16 bytes

    enum class INESFileFormat
    {
        iNES1d0,
        NES2d0,
    };
};

}; // namespace tn

#endif