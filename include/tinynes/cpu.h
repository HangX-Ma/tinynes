#ifndef TINYNES_CPU_H
#define TINYNES_CPU_H

#include <functional>
#include <vector>
#include <string>
#include <map>

namespace tn
{
// CPU memory map <https://www.nesdev.org/wiki/CPU_memory_map>

// The CPU expects interrupt vectors in a fixed place at the end of the cartridge space.
// - the NMI vector is at $FFFA
// - the reset vector at $FFFC
// - the IRQ and BRK vector at $FFFE

constexpr uint16_t NMI_VECTOR = 0xFFFA;
constexpr uint16_t RESET_VECTOR = 0xFFFC;
constexpr uint16_t IRQ_VECTOR = 0xFFFE;

class Bus;
class CPU
{
public:
    void connectBus(Bus *b) { bus_ = b; }

    // External event functions.
    void reset(); // Reset Interrupt - Forces CPU into known state
    void irq();   // Interrupt Request - Executes an instruction at a specific location
    void nmi();   // Non-Maskable Interrupt Request - As above, but cannot be disabled
    void clock();

private:
    struct Reg
    {
        uint8_t a{0x00};      // accumulator
        uint8_t x{0x00};      // index X
        uint8_t y{0x00};      // index y
        uint8_t st{0x00};     // stack pointer
        uint16_t pc{0x000};   // program counter
        uint8_t status{0x00}; // status register
    } reg_;

    /// @ref status flags <https://www.nesdev.org/wiki/Status_flags>
    enum FLAGS6502
    {
        C = (1 << 0), // Carry
        Z = (1 << 1), // Zero
        I = (1 << 2), // Interrupt Disable
        D = (1 << 3), // Decimal (unused)
        B = (1 << 4), // Break (No CPU effect)
        U = (1 << 5), // Unused (No CPU effect; always pushed as 1)
        V = (1 << 6), // Overflow
        N = (1 << 7), // Negative
    };

    bool getFlag(FLAGS6502 f);
    void setFlag(FLAGS6502 f, bool v);

private:
    Bus *bus_{nullptr};
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);

    uint8_t fetch();

private:
    uint8_t fetched_{0x00};     // Represents the working input value to the ALU
    uint16_t temp_{0x0000};     // A convenience variable used everywhere
    uint16_t addr_abs_{0x0000}; // All used memory addresses end up in here
    uint16_t addr_rel_{0x00};   // Represents absolute address following a branch
    uint8_t opcode_{0x00};      // Is the instruction byte
    uint8_t cycles_{0};         // Counts how many cycles the instruction has remaining
    uint32_t clock_count_{0};   // A global accumulation of the number of clocks

    // R650X, R651X data sheet introduces this instruction set opcode matrix
    struct Instruction
    {
        std::string mnemonic;
        uint8_t (CPU::*operate)() = nullptr;
        uint8_t (CPU::*addrmode)() = nullptr;
        uint8_t cycles{0};
    };
    const Instruction lookup_table_[256]{
        {"BRK", &CPU::BRK, &CPU::IMM, 7},
        {"ORA", &CPU::ORA, &CPU::IZX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::IMP, 3},
        {"ORA", &CPU::ORA, &CPU::ZP0, 3},
        {"ASL", &CPU::ASL, &CPU::ZP0, 5},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"PHP", &CPU::PHP, &CPU::IMP, 3},
        {"ORA", &CPU::ORA, &CPU::IMM, 2},
        {"ASL", &CPU::ASL, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"ORA", &CPU::ORA, &CPU::ABS, 4},
        {"ASL", &CPU::ASL, &CPU::ABS, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BPL", &CPU::BPL, &CPU::REL, 2},
        {"ORA", &CPU::ORA, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"ORA", &CPU::ORA, &CPU::ZPX, 4},
        {"ASL", &CPU::ASL, &CPU::ZPX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"CLC", &CPU::CLC, &CPU::IMP, 2},
        {"ORA", &CPU::ORA, &CPU::ABY, 4},
        {"???", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"ORA", &CPU::ORA, &CPU::ABX, 4},
        {"ASL", &CPU::ASL, &CPU::ABX, 7},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"JSR", &CPU::JSR, &CPU::ABS, 6},
        {"AND", &CPU::AND, &CPU::IZX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"BIT", &CPU::BIT, &CPU::ZP0, 3},
        {"AND", &CPU::AND, &CPU::ZP0, 3},
        {"ROL", &CPU::ROL, &CPU::ZP0, 5},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"PLP", &CPU::PLP, &CPU::IMP, 4},
        {"AND", &CPU::AND, &CPU::IMM, 2},
        {"ROL", &CPU::ROL, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"BIT", &CPU::BIT, &CPU::ABS, 4},
        {"AND", &CPU::AND, &CPU::ABS, 4},
        {"ROL", &CPU::ROL, &CPU::ABS, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BMI", &CPU::BMI, &CPU::REL, 2},
        {"AND", &CPU::AND, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"AND", &CPU::AND, &CPU::ZPX, 4},
        {"ROL", &CPU::ROL, &CPU::ZPX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"SEC", &CPU::SEC, &CPU::IMP, 2},
        {"AND", &CPU::AND, &CPU::ABY, 4},
        {"???", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"AND", &CPU::AND, &CPU::ABX, 4},
        {"ROL", &CPU::ROL, &CPU::ABX, 7},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"RTI", &CPU::RTI, &CPU::IMP, 6},
        {"EOR", &CPU::EOR, &CPU::IZX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::IMP, 3},
        {"EOR", &CPU::EOR, &CPU::ZP0, 3},
        {"LSR", &CPU::LSR, &CPU::ZP0, 5},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"PHA", &CPU::PHA, &CPU::IMP, 3},
        {"EOR", &CPU::EOR, &CPU::IMM, 2},
        {"LSR", &CPU::LSR, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"JMP", &CPU::JMP, &CPU::ABS, 3},
        {"EOR", &CPU::EOR, &CPU::ABS, 4},
        {"LSR", &CPU::LSR, &CPU::ABS, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BVC", &CPU::BVC, &CPU::REL, 2},
        {"EOR", &CPU::EOR, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"EOR", &CPU::EOR, &CPU::ZPX, 4},
        {"LSR", &CPU::LSR, &CPU::ZPX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"CLI", &CPU::CLI, &CPU::IMP, 2},
        {"EOR", &CPU::EOR, &CPU::ABY, 4},
        {"???", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"EOR", &CPU::EOR, &CPU::ABX, 4},
        {"LSR", &CPU::LSR, &CPU::ABX, 7},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"RTS", &CPU::RTS, &CPU::IMP, 6},
        {"ADC", &CPU::ADC, &CPU::IZX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::IMP, 3},
        {"ADC", &CPU::ADC, &CPU::ZP0, 3},
        {"ROR", &CPU::ROR, &CPU::ZP0, 5},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"PLA", &CPU::PLA, &CPU::IMP, 4},
        {"ADC", &CPU::ADC, &CPU::IMM, 2},
        {"ROR", &CPU::ROR, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"JMP", &CPU::JMP, &CPU::IND, 5},
        {"ADC", &CPU::ADC, &CPU::ABS, 4},
        {"ROR", &CPU::ROR, &CPU::ABS, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BVS", &CPU::BVS, &CPU::REL, 2},
        {"ADC", &CPU::ADC, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"ADC", &CPU::ADC, &CPU::ZPX, 4},
        {"ROR", &CPU::ROR, &CPU::ZPX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"SEI", &CPU::SEI, &CPU::IMP, 2},
        {"ADC", &CPU::ADC, &CPU::ABY, 4},
        {"???", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"ADC", &CPU::ADC, &CPU::ABX, 4},
        {"ROR", &CPU::ROR, &CPU::ABX, 7},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::IMP, 2},
        {"STA", &CPU::STA, &CPU::IZX, 6},
        {"???", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"STY", &CPU::STY, &CPU::ZP0, 3},
        {"STA", &CPU::STA, &CPU::ZP0, 3},
        {"STX", &CPU::STX, &CPU::ZP0, 3},
        {"???", &CPU::XXX, &CPU::IMP, 3},
        {"DEY", &CPU::DEY, &CPU::IMP, 2},
        {"???", &CPU::NOP, &CPU::IMP, 2},
        {"TXA", &CPU::TXA, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"STY", &CPU::STY, &CPU::ABS, 4},
        {"STA", &CPU::STA, &CPU::ABS, 4},
        {"STX", &CPU::STX, &CPU::ABS, 4},
        {"???", &CPU::XXX, &CPU::IMP, 4},
        {"BCC", &CPU::BCC, &CPU::REL, 2},
        {"STA", &CPU::STA, &CPU::IZY, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"STY", &CPU::STY, &CPU::ZPX, 4},
        {"STA", &CPU::STA, &CPU::ZPX, 4},
        {"STX", &CPU::STX, &CPU::ZPY, 4},
        {"???", &CPU::XXX, &CPU::IMP, 4},
        {"TYA", &CPU::TYA, &CPU::IMP, 2},
        {"STA", &CPU::STA, &CPU::ABY, 5},
        {"TXS", &CPU::TXS, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"???", &CPU::NOP, &CPU::IMP, 5},
        {"STA", &CPU::STA, &CPU::ABX, 5},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"LDY", &CPU::LDY, &CPU::IMM, 2},
        {"LDA", &CPU::LDA, &CPU::IZX, 6},
        {"LDX", &CPU::LDX, &CPU::IMM, 2},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"LDY", &CPU::LDY, &CPU::ZP0, 3},
        {"LDA", &CPU::LDA, &CPU::ZP0, 3},
        {"LDX", &CPU::LDX, &CPU::ZP0, 3},
        {"???", &CPU::XXX, &CPU::IMP, 3},
        {"TAY", &CPU::TAY, &CPU::IMP, 2},
        {"LDA", &CPU::LDA, &CPU::IMM, 2},
        {"TAX", &CPU::TAX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"LDY", &CPU::LDY, &CPU::ABS, 4},
        {"LDA", &CPU::LDA, &CPU::ABS, 4},
        {"LDX", &CPU::LDX, &CPU::ABS, 4},
        {"???", &CPU::XXX, &CPU::IMP, 4},
        {"BCS", &CPU::BCS, &CPU::REL, 2},
        {"LDA", &CPU::LDA, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"LDY", &CPU::LDY, &CPU::ZPX, 4},
        {"LDA", &CPU::LDA, &CPU::ZPX, 4},
        {"LDX", &CPU::LDX, &CPU::ZPY, 4},
        {"???", &CPU::XXX, &CPU::IMP, 4},
        {"CLV", &CPU::CLV, &CPU::IMP, 2},
        {"LDA", &CPU::LDA, &CPU::ABY, 4},
        {"TSX", &CPU::TSX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 4},
        {"LDY", &CPU::LDY, &CPU::ABX, 4},
        {"LDA", &CPU::LDA, &CPU::ABX, 4},
        {"LDX", &CPU::LDX, &CPU::ABY, 4},
        {"???", &CPU::XXX, &CPU::IMP, 4},
        {"CPY", &CPU::CPY, &CPU::IMM, 2},
        {"CMP", &CPU::CMP, &CPU::IZX, 6},
        {"???", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"CPY", &CPU::CPY, &CPU::ZP0, 3},
        {"CMP", &CPU::CMP, &CPU::ZP0, 3},
        {"DEC", &CPU::DEC, &CPU::ZP0, 5},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"INY", &CPU::INY, &CPU::IMP, 2},
        {"CMP", &CPU::CMP, &CPU::IMM, 2},
        {"DEX", &CPU::DEX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"CPY", &CPU::CPY, &CPU::ABS, 4},
        {"CMP", &CPU::CMP, &CPU::ABS, 4},
        {"DEC", &CPU::DEC, &CPU::ABS, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BNE", &CPU::BNE, &CPU::REL, 2},
        {"CMP", &CPU::CMP, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"CMP", &CPU::CMP, &CPU::ZPX, 4},
        {"DEC", &CPU::DEC, &CPU::ZPX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"CLD", &CPU::CLD, &CPU::IMP, 2},
        {"CMP", &CPU::CMP, &CPU::ABY, 4},
        {"NOP", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"CMP", &CPU::CMP, &CPU::ABX, 4},
        {"DEC", &CPU::DEC, &CPU::ABX, 7},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"CPX", &CPU::CPX, &CPU::IMM, 2},
        {"SBC", &CPU::SBC, &CPU::IZX, 6},
        {"???", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"CPX", &CPU::CPX, &CPU::ZP0, 3},
        {"SBC", &CPU::SBC, &CPU::ZP0, 3},
        {"INC", &CPU::INC, &CPU::ZP0, 5},
        {"???", &CPU::XXX, &CPU::IMP, 5},
        {"INX", &CPU::INX, &CPU::IMP, 2},
        {"SBC", &CPU::SBC, &CPU::IMM, 2},
        {"NOP", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::SBC, &CPU::IMP, 2},
        {"CPX", &CPU::CPX, &CPU::ABS, 4},
        {"SBC", &CPU::SBC, &CPU::ABS, 4},
        {"INC", &CPU::INC, &CPU::ABS, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BEQ", &CPU::BEQ, &CPU::REL, 2},
        {"SBC", &CPU::SBC, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"SBC", &CPU::SBC, &CPU::ZPX, 4},
        {"INC", &CPU::INC, &CPU::ZPX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 6},
        {"SED", &CPU::SED, &CPU::IMP, 2},
        {"SBC", &CPU::SBC, &CPU::ABY, 4},
        {"NOP", &CPU::NOP, &CPU::IMP, 2},
        {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::IMP, 4},
        {"SBC", &CPU::SBC, &CPU::ABX, 4},
        {"INC", &CPU::INC, &CPU::ABX, 7},
        {"???", &CPU::XXX, &CPU::IMP, 7},
    };

    // clang-format off
    // NOLINTBEGIN
private:
    // CPU addressing modes <https://www.nesdev.org/wiki/CPU_addressing_modes>
    // The returned value indicates whether there is the "oops" time cycle.
    // Store instructions always have this "oops" cycle:
    //      The CPU first reads from the partially added address and then writes to
    //      the correct address. The same thing happens on (d),y indirect addressing.
    uint8_t IMP(); // implied addressing
    uint8_t IMM(); // immediate addressing
    uint8_t ZP0(); // zero page addressing
    uint8_t ZPX(); // indexed zero page X addressing
    uint8_t ZPY(); // indexed zero page Y addressing
    uint8_t REL(); // relative addressing
    uint8_t ABS(); // absolute addressing
    uint8_t ABX(); // indexed absolute X addressing
    uint8_t ABY(); // indexed absolute Y addressing
    uint8_t IND(); // absolute indirect
    uint8_t IZX(); // indexed indirect X addressing
    uint8_t IZY(); // indexed indirect Y addressing

private:
    // Opcodes
    // There are 56 "legitimate" opcodes provided by the 6502 CPU. I
    // have not modelled "unofficial" opcodes. As each opcode is
    // defined by 1 byte, there are potentially 256 possible codes.
    //
    // These functions return 0 normally, but some are capable of
    // requiring more clock cycles when executed under certain
    // conditions combined with certain addressing modes. If that is
    // the case, they return 1.
    uint8_t ADC();  uint8_t AND();  uint8_t ASL();  uint8_t BCC();
    uint8_t BCS();  uint8_t BEQ();  uint8_t BIT();  uint8_t BMI();
    uint8_t BNE();  uint8_t BPL();  uint8_t BRK();  uint8_t BVC();
    uint8_t BVS();  uint8_t CLC();  uint8_t CLD();  uint8_t CLI();
    uint8_t CLV();  uint8_t CMP();  uint8_t CPX();  uint8_t CPY();
    uint8_t DEC();  uint8_t DEX();  uint8_t DEY();  uint8_t EOR();
    uint8_t INC();  uint8_t INX();  uint8_t INY();  uint8_t JMP();
    uint8_t JSR();  uint8_t LDA();  uint8_t LDX();  uint8_t LDY();
    uint8_t LSR();  uint8_t NOP();  uint8_t ORA();  uint8_t PHA();
    uint8_t PHP();  uint8_t PLA();  uint8_t PLP();  uint8_t ROL();
    uint8_t ROR();  uint8_t RTI();  uint8_t RTS();  uint8_t SBC();
    uint8_t SEC();  uint8_t SED();  uint8_t SEI();  uint8_t STA();
    uint8_t STX();  uint8_t STY();  uint8_t TAX();  uint8_t TAY();
    uint8_t TSX();  uint8_t TXA();  uint8_t TXS();  uint8_t TYA();

    // Capture all "unofficial" opcodes with this function.
    // It is functionally identical to a NOP
    uint8_t XXX();
    // NOLINTEND
};

} // namespace tn

#endif