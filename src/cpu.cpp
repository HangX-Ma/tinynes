#include "tinynes/cpu.h"
#include "tinynes/bus.h"
#include "tinynes/utils.h"

#include <spdlog/fmt/fmt.h>

namespace tn
{

// STATUS FLAG FUNCTION
bool CPU::getFlag(FLAGS6502 f) { return (reg_.status & f) > 0; }
void CPU::setFlag(FLAGS6502 f, bool v)
{
    if (v) {
        reg_.status |= f;
    }
    else {
        reg_.status &= ~f;
    }
}

// BUS ACCESS
uint8_t CPU::read(uint16_t addr) { return bus_->read(addr, false); }
void CPU::write(uint16_t addr, uint8_t data) { bus_->write(addr, data); }

// EXTERNAL EVENT
// CPU power up state: <https://www.nesdev.org/wiki/CPU_power_up_state#cite_note-2>
void CPU::reset()
{
    addr_abs_ = RESET_VECTOR;
    uint16_t lo = read(addr_abs_);
    uint16_t hi = read(addr_abs_ + 1);
    reg_.pc = (hi << 8) | lo;

    reg_.st = 0xFD;
    reg_.status = 0x00 | U;

    // reset internal register
    reg_.a = 0;
    reg_.x = 0;
    reg_.y = 0;

    // Clear internal variables
    addr_rel_ = 0x0000;
    addr_abs_ = 0x0000;
    fetched_ = 0x00;

    // Reset takes time
    cycles_ = 8;
}

// CPU interrupts: <https://www.nesdev.org/wiki/CPU_interrupts>
// - Branch instructions and interrupts
// - IRQ and NMI tick-by-tick execution
void CPU::irq()
{
    // If interrupts are allowed
    if (getFlag(I)) {
        // push PCH on stack, decrement stack pointer
        write(0x100 + reg_.st, (reg_.pc & 0xFF00) >> 8);
        reg_.st -= 1;
        // push PCL on stack, decrement stack pointer
        write(0x100 + reg_.st, reg_.pc & 0x00FF);
        reg_.st -= 1;
        // push status register on stack, decrement stack pointer
        write(0x100 + reg_.st, reg_.status);
        reg_.st -= 1;

        setFlag(B, false);
        setFlag(U, true); // always set to 1
        setFlag(I, true); // set I flag to clear interrupt state

        // fetch PCL and PCH from IRQ vector address
        addr_abs_ = IRQ_VECTOR;
        uint16_t lo = read(addr_abs_);
        uint16_t hi = read(addr_abs_ + 1);
        reg_.pc = (hi << 8) | lo;

        // IRQ time cycles
        cycles_ = 7;
    }
}

// just change the JMP address to NMI_VECTOR
void CPU::nmi()
{
    // push PCH on stack, decrement stack pointer
    write(0x100 + reg_.st, (reg_.pc & 0xFF00) >> 8);
    reg_.st -= 1;
    // push PCL on stack, decrement stack pointer
    write(0x100 + reg_.st, reg_.pc & 0x00FF);
    reg_.st -= 1;
    // push status register on stack, decrement stack pointer
    write(0x100 + reg_.st, reg_.status);
    reg_.st -= 1;

    setFlag(B, false);
    setFlag(U, true); // always set to 1
    setFlag(I, true); // set I flag to clear interrupt state

    // fetch PCL and PCH from NMI vector address
    addr_abs_ = NMI_VECTOR;
    uint16_t lo = read(addr_abs_);
    uint16_t hi = read(addr_abs_ + 1);
    reg_.pc = (hi << 8) | lo;

    // NMI time cycles
    cycles_ = 8;
}

void CPU::clock()
{
    // the next instruction is ready to be executed.
    if (cycles_ == 0) {
        // read next instruction byte to acquire the info about how to implement this instruction.
        opcode_ = read(reg_.pc);
        reg_.pc += 1;

        // flag U is always 1
        setFlag(U, true);

        // get next instruction cycles
        cycles_ = lookup_table_[opcode_].cycles;

        uint8_t additional_cycle1 = (this->*lookup_table_[opcode_].addrmode)();
        uint8_t additional_cycle2 = (this->*lookup_table_[opcode_].operate)();
        cycles_ += (additional_cycle1 + additional_cycle2);

        setFlag(U, true);
    }
    // update clock
    clock_count_ += 1;
    cycles_ -= 1;
}

bool CPU::complete() { return cycles_ == 0; }

void CPU::disassemble(uint16_t addr_begin, uint16_t addr_end, ASMMap &asm_map)
{
    uint32_t line_addr;
    uint32_t addr = addr_begin;
    uint8_t value = 0x00;
    uint8_t lo = 0x00;
    uint8_t hi = 0x00;

    while (addr <= addr_end) {
        line_addr = addr;
        std::string instruction = fmt::format("${}: ", tn::Utils::numToHex(addr, 4));

        // read instruction
        uint8_t opcode = bus_->read(addr, true);
        addr += 1;
        instruction.append(lookup_table_[opcode].mnemonic);
        instruction.append(" ");

        if (lookup_table_[opcode].addrmode == &CPU::IMP) {
            instruction.append(" {{IMP}}");
        }
        else if (lookup_table_[opcode].addrmode == &CPU::IMM) {
            value = bus_->read(addr, true);
            addr += 1;
            instruction.append(fmt::format("#${} {{IMM}}", tn::Utils::numToHex(value, 2)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::ZP0) {
            lo = bus_->read(addr, true);
            addr += 1;
            hi = 0x00;
            instruction.append(fmt::format("${} {{ZP0}}", tn::Utils::numToHex(lo, 2)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::ZPX) {
            lo = bus_->read(addr, true);
            addr += 1;
            hi = 0x00;
            instruction.append(fmt::format("${}, X {{ZPX}}", tn::Utils::numToHex(lo, 2)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::ZPY) {
            lo = bus_->read(addr, true);
            addr += 1;
            hi = 0x00;
            instruction.append(fmt::format("(${}), Y {{ZPY}}", tn::Utils::numToHex(lo, 2)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::IZX) {
            lo = bus_->read(addr, true);
            addr += 1;
            hi = 0x00;
            instruction.append(fmt::format("(${}), X {{IZX}}", tn::Utils::numToHex(lo, 2)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::IZY) {
            lo = bus_->read(addr, true);
            addr += 1;
            hi = 0x00;
            instruction.append(fmt::format("${}, Y {{IZY}}", tn::Utils::numToHex(lo, 2)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::ABS) {
            lo = bus_->read(addr, true);
            addr += 1;
            hi = bus_->read(addr, true);
            addr += 1;
            instruction.append(fmt::format(
                "${} {{ABS}}", tn::Utils::numToHex(static_cast<uint16_t>(hi << 8 | lo), 4)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::ABX) {
            lo = bus_->read(addr, true);
            addr += 1;
            hi = bus_->read(addr, true);
            addr += 1;
            instruction.append(fmt::format(
                "${}, X {{ABX}}", tn::Utils::numToHex(static_cast<uint16_t>(hi << 8 | lo), 4)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::ABY) {
            lo = bus_->read(addr, true);
            addr += 1;
            hi = bus_->read(addr, true);
            addr += 1;
            instruction.append(fmt::format(
                "${}, Y {{ABY}}", tn::Utils::numToHex(static_cast<uint16_t>(hi << 8 | lo), 4)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::IND) {
            lo = bus_->read(addr, true);
            addr += 1;
            hi = bus_->read(addr, true);
            addr += 1;
            instruction.append(fmt::format(
                "(${}) {{IND}}", tn::Utils::numToHex(static_cast<uint16_t>(hi << 8 | lo), 4)));
        }
        else if (lookup_table_[opcode].addrmode == &CPU::REL) {
            value = bus_->read(addr, true);
            addr += 1;
            instruction.append(fmt::format("${} [${}] {{REL}}", tn::Utils::numToHex(value, 2),
                                           tn::Utils::numToHex(addr + value, 4)));
        }
        asm_map[line_addr] = instruction;
    }
}

// ADDRESSING MODES

// Mode: Implied, 1 bytes
// Instructions like RTS or CLC have no address operand, the destination of results are implied.
// Generally do nothing in this mode, but some instruction like PHA(push accumulator) may use
// the 'Accumulator Register'. So we just move this register into 'fetched' variable for potential
// usage.
uint8_t CPU::IMP()
{
    fetched_ = reg_.a;
    return 0;
}

// Mode: Immediate, 2 bytes
// Use this mode instruction occupies 2 bytes. One byte for instruction itself.
// Another for immediate number.
uint8_t CPU::IMM()
{
    addr_abs_ = reg_.pc;
    reg_.pc += 1;
    return 0;
}

// Mode: Zero Page, 2 bytes
// Zero page mode Fetches the value from an 8-bit address on the zero page. In 6502, one page is
// 256 bytes size. That means we can divide the 64 KB address into two part: High 8 bits and Low 8
// bits. The higher 8 bits indicate the page number, and the lower 8 bits indicate the offset in
// pages.
uint8_t CPU::ZP0()
{
    // read the offset value stored in next byte
    addr_abs_ = read(reg_.pc);
    addr_abs_ &= 0x00FF;
    reg_.pc += 1;
    return 0;
}

// Mode: Zero page indexed with X, 2 bytes
uint8_t CPU::ZPX()
{
    addr_abs_ = read(reg_.pc) + reg_.x;
    addr_abs_ &= 0x00FF;
    reg_.pc += 1;
    return 0;
}

// Mode: Zero page indexed with Y, 2 bytes
uint8_t CPU::ZPY()
{
    addr_abs_ = read(reg_.pc) + reg_.y;
    addr_abs_ &= 0x00FF;
    reg_.pc += 1;
    return 0;
}
// Mode: Relative, 2 bytes
// The address must reside within -128 to +127 of the branch instruction.
uint8_t CPU::REL()
{
    // read the signed offset from next byte
    addr_rel_ = read(reg_.pc);
    reg_.pc += 1;

    // 'read' function return an unsigned number. So we need to check the
    // sign bit to determine if the number is negative or not. If it is,
    // we need to make the 'addr_rel_' represented as a signed 16 bits
    // negative number.
    if ((addr_rel_ & 0x80) > 0) {
        addr_rel_ |= 0xFF00;
    }
    return 0;
}

// Mode: Absolute, 3 bytes
// Fetches the value from a 16-bit address anywhere in memory
// (16 bits can access maximum 64 KB memory address space).
uint8_t CPU::ABS()
{
    uint16_t lo = read(reg_.pc);
    reg_.pc += 1;
    uint16_t hi = read(reg_.pc);
    reg_.pc += 1;
    addr_abs_ = (hi << 8) | lo;
    return 0;
}

// Mode: Absolute indexed with X, 4 bytes
// This mode may influence the carry out and generate 'oops' cycle
uint8_t CPU::ABX()
{
    uint16_t lo = read(reg_.pc);
    reg_.pc += 1;
    uint16_t hi = read(reg_.pc);
    reg_.pc += 1;
    addr_abs_ = (hi << 8) | lo;
    addr_abs_ += reg_.x;

    // previous addition operation change the 'hi' byte, causing carry out.
    if ((addr_abs_ & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

// Mode: Absolute indexed with Y, 4 bytes
// This mode may influence the carry out and generate 'oops' cycle
uint8_t CPU::ABY()
{
    uint16_t lo = read(reg_.pc);
    reg_.pc += 1;
    uint16_t hi = read(reg_.pc);
    reg_.pc += 1;
    addr_abs_ = (hi << 8) | lo;
    addr_abs_ += reg_.y;

    // previous addition operation change the 'hi' byte, causing carry out.
    if ((addr_abs_ & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

// Mode: Indexed indirect
// The supplied 16-bit address is read to get the actual 16-bit address.
uint8_t CPU::IND()
{
    uint16_t ptr_lo = read(reg_.pc);
    reg_.pc += 1;
    uint16_t ptr_hi = read(reg_.pc);
    reg_.pc += 1;
    uint16_t ptr = (ptr_hi << 8) | ptr_lo;

    // we may encounter page boundary fault because next byte may cross two pages
    if (ptr_lo == 0x00FF) {
        addr_abs_ = read(ptr & 0xFF00) << 8 | read(ptr);
    }
    else {
        addr_abs_ = read(ptr + 1) << 8 | read(ptr);
    }
    return 0;
}

// Mode: Indexed indirect with X
// The 8 bits X offset directly adds to read pointer address
uint8_t CPU::IZX()
{
    uint16_t ptr = read(reg_.pc);
    reg_.pc += 1;

    uint16_t lo = read((ptr + static_cast<uint16_t>(reg_.x)) & 0x00FF);
    uint16_t hi = read((ptr + static_cast<uint16_t>(reg_.x) + 1) & 0x00FF);

    addr_abs_ = (hi << 8) | lo;
    return 0;
}

// Mode: Indexed indirect with Y
// The actual 16-bit address is read, and the contents of Y Register is added to it to offset it.
// This operation may result in page crossing and need to add 'oops' cycle.
uint8_t CPU::IZY()
{
    uint16_t ptr = read(reg_.pc);
    reg_.pc += 1;

    uint16_t lo = read(ptr & 0x00FF);
    uint16_t hi = read((ptr + 1) & 0x00FF);

    addr_abs_ = (hi << 8) | lo;
    addr_abs_ += reg_.y;

    if ((addr_abs_ & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

// Some instruction like PHA(push accumulator), implied type, have no additional data
// required. What we need to do is getting data from accumulator. For other instructions,
// the data is stored in the 'addr_abs_'. So just read from it.
uint8_t CPU::fetch()
{
    if (!(lookup_table_[opcode_].addrmode == &CPU::IMP)) {
        fetched_ = read(addr_abs_);
    }
    return fetched_;
}

// INSTRUCTION IMPLEMENTATIONS
// 6502 Instructions in Detail: <https://www.masswerk.at/6502/6502_instruction_set.html#ADC>
// The normal operation order:
// 1) Fetch the data you are working with
// 2) Perform calculation
// 3) Store the result in desired place
// 4) Set Flags of the status register
// 5) Return if instruction has potential to require additional clock cycle

// Description:     Add Memory to Accumulator with Carry
// Function:        A+M+C -> A
// Condition code:  N, Z, C, V

// The most significant bit of A, M, R consist of the following truth table
// A  M  R(result) | V |
// 0  0  0 | 0 |
// 0  0  1 | 1 |
// 0  1  0 | 0 |
// 0  1  1 | 0 |
// 1  0  0 | 0 |
// 1  0  1 | 0 |
// 1  1  0 | 1 |
// 1  1  1 | 0 |
//
// V = A'M'R + AMR' -> V = (A & M) ^ R
uint8_t CPU::ADC()
{
    // Grab the data that we are adding to the accumulator
    fetch();

    // A + M + C
    temp_ = static_cast<uint16_t>(reg_.a) + static_cast<uint16_t>(fetched_)
            + static_cast<uint16_t>(getFlag(C));

    setFlag(C, temp_ > 0xFF);
    setFlag(N, (temp_ & 0x80) != 0);
    setFlag(Z, (temp_ & 0x00FF) == 0);
    setFlag(V,
            (((static_cast<uint16_t>(reg_.a) & static_cast<uint16_t>(fetched_)) ^ temp_) & 0x0080)
                != 0);
    reg_.a = temp_ & 0x0080;

    // potentially require 'oops' cycle if page boundary crossed
    return 1;
}

// Description: AND Memory with Accumulator
// Function: A AND M -> A
// Condition code: N, Z
uint8_t CPU::AND()
{
    fetch();

    reg_.a = reg_.a & fetched_;

    setFlag(Z, reg_.a == 0);
    setFlag(N, (reg_.a & 0x80) != 0);

    return 1;
}

// Description:     Shift Left One Bit (Memory or Accumulator)
// Function:        C <- [76543210] <- 0
// Condition code:  N, Z, C
uint8_t CPU::ASL()
{
    fetch();

    temp_ = static_cast<uint16_t>(fetched_) << 1;

    setFlag(C, temp_ > 0xFF);
    setFlag(Z, (temp_ & 0x00FF) == 0);
    setFlag(N, (temp_ & 0x80) != 0);

    // write back to source, accumulator or memory
    if (lookup_table_[opcode_].addrmode == &CPU::IMP) {
        reg_.a = temp_ & 0x00FF;
    }
    else {
        write(addr_abs_, temp_ & 0x00FF);
    }

    return 0;
}

// Description: Branch on Carry Clear
// Function:    branch on C = 0, relative
uint8_t CPU::BCC()
{
    if (!getFlag(C)) {
        cycles_ += 1;
        addr_abs_ = reg_.pc + addr_rel_;

        if ((addr_abs_ & 0xFF00) != (reg_.pc & 0xFF00)) {
            cycles_ += 1;
        }
        reg_.pc = addr_abs_;
    }
    return 0;
}

// Description: Branch on Carry Set
// Function:    branch on C = 1, relative
uint8_t CPU::BCS()
{
    if (getFlag(C)) {
        cycles_ += 1;
        addr_abs_ = reg_.pc + addr_rel_;

        if ((addr_abs_ & 0xFF00) != (reg_.pc & 0xFF00)) {
            cycles_ += 1;
        }
        reg_.pc = addr_abs_;
    }
    return 0;
}

// Description: Branch on Result Zero
// Function:    branch on Z = 1, relative
uint8_t CPU::BEQ()
{
    if (getFlag(Z)) {
        cycles_ += 1;
        addr_abs_ = reg_.pc + addr_rel_;

        if ((addr_abs_ & 0xFF00) != (reg_.pc & 0xFF00)) {
            cycles_ += 1;
        }
        reg_.pc = addr_abs_;
    }
    return 0;
}

// Description:     Test Bits in Memory with Accumulator
//                  bits 7 and 6 of operand are transferred to bit 7 and 6 of SR (N,V);
//                  the zero-flag is set according to the result of the operand AND
//                  the accumulator (set, if the result is zero, unset otherwise).
//                  This allows a quick check of a few bits at once without affecting
//                  any of the registers, other than the status register (SR).
// Function:        A AND M, M7 -> N, M6 -> V
// Condition code:  N, Z, V
uint8_t CPU::BIT()
{
    fetch();

    temp_ = reg_.a & fetched_;
    setFlag(Z, (temp_ & 0x00FF) == 0);
    setFlag(N, (fetched_ & (1 << 7)) != 0);
    setFlag(V, (fetched_ & (1 << 6)) != 0);

    return 0;
}

// Description: Branch on Result Minus
// Function:    branch on N = 1
uint8_t CPU::BMI()
{
    if (getFlag(N)) {
        cycles_ += 1;
        addr_abs_ = reg_.pc + addr_rel_;

        if ((addr_abs_ & 0xFF00) != (reg_.pc & 0xFF00)) {
            cycles_ += 1;
        }
        reg_.pc = addr_abs_;
    }
    return 0;
}

// Description: Branch on Result not Zero
// Function:    branch on Z = 0
uint8_t CPU::BNE()
{
    if (!getFlag(Z)) {
        cycles_ += 1;
        addr_abs_ = reg_.pc + addr_rel_;

        if ((addr_abs_ & 0xFF00) != (reg_.pc & 0xFF00)) {
            cycles_ += 1;
        }
        reg_.pc = addr_abs_;
    }
    return 0;
}

// Description: Branch on Result Plus
// Function:    branch on N = 0
uint8_t CPU::BPL()
{
    if (!getFlag(N)) {
        cycles_ += 1;
        addr_abs_ = reg_.pc + addr_rel_;

        if ((addr_abs_ & 0xFF00) != (reg_.pc & 0xFF00)) {
            cycles_ += 1;
        }
        reg_.pc = addr_abs_;
    }
    return 0;
}

// Description: Force Break
//              BRK initiates a software interrupt similar to a hardware
//              interrupt (IRQ). The return address pushed to the stack is
//              PC+2, providing an extra byte of spacing for a break mark
//              (identifying a reason for the break.)
//              The status register will be pushed to the stack with the break
//              flag set to 1. However, when retrieved during RTI or by a PLP
//              instruction, the break flag will be ignored.
//              The interrupt disable flag is not set automatically.
// Function:    interrupt, push PC+2, push SR
// Condition code: B(virtual), I(1)
uint8_t CPU::BRK()
{
    reg_.pc += 1;

    setFlag(I, true);
    write(0x100 + reg_.st, (reg_.pc >> 8) & 0x00FF);
    reg_.st -= 1;
    write(0x100 + reg_.st, reg_.pc & 0x00FF);
    reg_.st -= 1;

    setFlag(B, true);
    write(0x100 + reg_.st, reg_.status);
    reg_.st -= 1;
    setFlag(B, false);

    reg_.pc = static_cast<uint16_t>(read(IRQ_VECTOR))
              | (static_cast<uint16_t>(read(IRQ_VECTOR + 1)) << 8);

    return 0;
}

// Description:     Branch on Overflow Clear
// Function:        branch on V = 0
uint8_t CPU::BVC()
{
    if (!getFlag(V)) {
        cycles_ += 1;
        addr_abs_ = reg_.pc + addr_rel_;

        if ((addr_abs_ & 0xFF00) != (reg_.pc & 0xFF00)) {
            cycles_ += 1;
        }
        reg_.pc = addr_abs_;
    }
    return 0;
}

// Description: Branch on Overflow Set
// Function:    branch on V = 1
uint8_t CPU::BVS()
{
    if (getFlag(V)) {
        cycles_ += 1;
        addr_abs_ = reg_.pc + addr_rel_;

        if ((addr_abs_ & 0xFF00) != (reg_.pc & 0xFF00)) {
            cycles_ += 1;
        }
        reg_.pc = addr_abs_;
    }
    return 0;
}

// Description:     Clear Carry Flag
// Function:        0 -> C
// Condition code:  C(0)
uint8_t CPU::CLC()
{
    setFlag(C, false);
    return 0;
}

// Description:     Clear Decimal Mode
// Function:        0 -> D
// Condition code:  D(0)
uint8_t CPU::CLD()
{
    setFlag(D, false);
    return 0;
}

// Description:     Clear Interrupt Disable Bit
// Function:        0 -> I
// Condition code:  I(0)
uint8_t CPU::CLI()
{
    setFlag(I, false);
    return 0;
}

// Description:     Clear Overflow Flag
// Function:        0 -> V
// Condition code:  V(0)
uint8_t CPU::CLV()
{
    setFlag(V, false);
    return 0;
}

// Description:     Compare Memory with Accumulator
// Function:        A - M
// Condition code:  N, Z, C
uint8_t CPU::CMP()
{
    fetch();
    temp_ = static_cast<uint16_t>(reg_.a) - static_cast<uint16_t>(fetched_);
    setFlag(N, (temp_ & 0x80) != 0);
    setFlag(Z, (temp_ & 0x00FF) == 0);
    // C is set if there is an unsigned overflow
    setFlag(C, reg_.a >= fetched_);

    return 1;
}

// Description:     Compare Memory and Index X
// Function:        X - M
// Condition code:  N, Z, C
uint8_t CPU::CPX()
{
    fetch();
    temp_ = static_cast<uint16_t>(reg_.x) - static_cast<uint16_t>(fetched_);
    setFlag(N, (temp_ & 0x80) != 0);
    setFlag(Z, (temp_ & 0x00FF) == 0);
    // C is set if there is an unsigned overflow
    setFlag(C, reg_.x >= fetched_);

    return 0;
}

// Description:     Compare Memory and Index Y
// Function:        Y - M
// Condition code:  N, Z, C
uint8_t CPU::CPY()
{
    fetch();
    temp_ = static_cast<uint16_t>(reg_.y) - static_cast<uint16_t>(fetched_);
    setFlag(N, (temp_ & 0x80) != 0);
    setFlag(Z, (temp_ & 0x00FF) == 0);
    // C is set if there is an unsigned overflow
    setFlag(C, reg_.y >= fetched_);

    return 0;
}

// Description:     Decrement Memory by One
// Function:        M - 1 -> M
// Condition code:  N, Z
uint8_t CPU::DEC()
{
    fetch();
    temp_ = static_cast<uint16_t>(fetched_) - 1;
    write(addr_abs_, temp_ & 0x00FF);
    setFlag(N, (temp_ & 0x80) != 0);
    setFlag(Z, (temp_ & 0x00FF) == 0);

    return 0;
}

// Description:     Decrement Index X by One
// Function:        X - 1 -> X
// Condition code:  N, Z
uint8_t CPU::DEX()
{
    reg_.x -= 1;
    setFlag(N, (reg_.x & 0x80) != 0);
    setFlag(Z, reg_.x == 0);

    return 0;
}

// Description:     Decrement Index Y by One
// Function:        Y - 1 -> Y
// Condition code:  N, Z
uint8_t CPU::DEY()
{
    reg_.y -= 1;
    setFlag(N, (reg_.y & 0x80) != 0);
    setFlag(Z, reg_.y == 0);

    return 0;
}

// Description:     Exclusive-OR Memory with Accumulator
// Function:        A EOR M -> A
// Condition code:  N, Z
uint8_t CPU::EOR()
{
    fetch();
    reg_.a = reg_.a ^ fetched_;
    setFlag(N, (reg_.y & 0x80) != 0);
    setFlag(Z, reg_.y == 0);
    return 1;
}

// Description:     Increment Memory by One
// Function:        M + 1 -> M
// Condition code:  N, Z
uint8_t CPU::INC()
{
    fetch();
    temp_ = fetched_ + 1;
    write(addr_abs_, temp_ & 0x00FF);
    setFlag(N, (temp_ & 0x80) != 0);
    setFlag(Z, (temp_ & 0x00FF) == 0);
    return 0;
}

// Description:     Increment Index X by One
// Function:        X + 1 -> X
// Condition code:  N, Z
uint8_t CPU::INX()
{
    reg_.x += 1;
    setFlag(N, (reg_.x & 0x80) != 0);
    setFlag(Z, reg_.x == 0);

    return 0;
}

// Description:     Increment Index Y by One
// Function:        Y + 1 -> Y
// Condition code:  N, Z
uint8_t CPU::INY()
{
    reg_.y += 1;
    setFlag(N, (reg_.y & 0x80) != 0);
    setFlag(Z, reg_.y == 0);

    return 0;
}

// Description:     Jump to New Location
// Function:        (PC+1) -> PCL
//                  (PC+2) -> PCH
uint8_t CPU::JMP()
{
    reg_.pc = addr_abs_;
    return 0;
}

// Description:     Jump to New Location Saving Return Address
// Function:        push (PC+2),
//                  (PC+1) -> PCL
//                  (PC+2) -> PCH
uint8_t CPU::JSR()
{
    reg_.pc += 1;

    write(0x0100 + reg_.st, (reg_.pc >> 8) & 0x00FF);
    reg_.st -= 1;
    write(0x0100 + reg_.st, reg_.pc & 0x00FF);
    reg_.st -= 1;

    reg_.pc = addr_abs_;
    return 0;
}

// Description:     Load Accumulator with Memory
// Function:        M -> A
// Condition code:  N, Z
uint8_t CPU::LDA()
{
    fetch();
    reg_.a = fetched_;
    setFlag(N, (reg_.a & 0x80) != 0);
    setFlag(Z, reg_.a == 0);

    return 1;
}

// Description:     Load Index X with Memory
// Function:        M -> X
// Condition code:  N, Z
uint8_t CPU::LDX()
{
    fetch();
    reg_.x = fetched_;
    setFlag(N, (reg_.x & 0x80) != 0);
    setFlag(Z, reg_.x == 0);
    return 1;
}

// Description:     Load Index Y with Memory
// Function:        M -> Y
// Condition code:  N, Z
uint8_t CPU::LDY()
{
    fetch();
    reg_.y = fetched_;
    setFlag(N, (reg_.y & 0x80) != 0);
    setFlag(Z, reg_.y == 0);
    return 1;
}

// Description:     Shift One Bit Right (Memory or Accumulator)
// Function:        0 -> [76543210] -> C
// Condition code:  N(0), Z, C
uint8_t CPU::LSR()
{
    fetch();
    temp_ = fetched_ >> 1;
    setFlag(C, (fetched_ & 1) != 0);
    setFlag(Z, (temp_ & 0x00FF) == 0);
    setFlag(N, (temp_ & 0x80) != 0);

    if (lookup_table_[opcode_].addrmode == &CPU::IMP) {
        reg_.a = temp_ & 0x00FF;
    }
    else {
        write(addr_abs_, temp_ & 0x00FF);
    }
    return 0;
}

// Description: No Operation
uint8_t CPU::NOP()
{
    // https://wiki.nesdev.com/w/index.php/CPU_unofficial_opcodes
    switch (opcode_) {
    case 0x1C:
    case 0x3C:
    case 0x5C:
    case 0x7C:
    case 0xDC:
    case 0xFC:
        return 1;
        break;
    }
    return 0;
}

// Description:     OR Memory with Accumulator
// Function:        A OR M -> A
// Condition code:  N, Z
uint8_t CPU::ORA()
{
    fetch();
    reg_.a = reg_.a | fetched_;
    setFlag(N, (reg_.a & 0x80) != 0);
    setFlag(Z, reg_.a == 0);

    return 1;
}

// Description: Push Accumulator on Stack
// Function:    push A
uint8_t CPU::PHA()
{
    write(0x100 + reg_.st, reg_.a);
    reg_.st -= 1;

    return 0;
}

// Description: Push Processor Status on Stack
// Function:    push SR
// Note:        Break flag is set to 1 before push ??
uint8_t CPU::PHP()
{
    write(0x100 + reg_.st, reg_.status | B | U);
    reg_.st -= 1;
    setFlag(B, false);
    setFlag(U, false);

    return 0;
}

// Description:     Pull Accumulator from Stack
// Function:        pull A
// Condition code:  N, Z
uint8_t CPU::PLA()
{
    reg_.st += 1;
    reg_.a = read(0x0100 + reg_.st);
    setFlag(N, (reg_.a & 0x80) != 0);
    setFlag(Z, reg_.a == 0);

    return 0;
}

// Description: Pull Processor Status from Stack
// Function:    pull SR
uint8_t CPU::PLP()
{
    reg_.st += 1;
    reg_.status = read(0x0100 + reg_.st);
    setFlag(U, true);

    return 0;
}

// Description:     Rotate One Bit Left (Memory or Accumulator)
// Function:        C <- [76543210] <- C
// Condition code:  N, Z, C
uint8_t CPU::ROL()
{
    fetch();
    temp_ = static_cast<uint16_t>(fetched_ << 1) | static_cast<uint16_t>(getFlag(C));
    setFlag(C, (temp_ & 0xFF00) != 0);
    setFlag(Z, (temp_ & 0x00FF) == 0);
    setFlag(N, (temp_ & 0x80) != 0);
    if (lookup_table_[opcode_].addrmode == &CPU::IMP) {
        reg_.a = temp_ & 0x00FF;
    }
    else {
        write(addr_abs_, temp_ & 0x00FF);
    }
    return 0;
}

// Description:     Rotate One Bit Right (Memory or Accumulator)
// Function:        C -> [76543210] -> C
// Condition code:  N, Z, C
uint8_t CPU::ROR()
{
    fetch();
    temp_ = static_cast<uint16_t>(fetched_ >> 1) | static_cast<uint16_t>(getFlag(C)) << 7;
    setFlag(C, (fetched_ & 1) != 0);
    setFlag(Z, (temp_ & 0x00FF) == 0);
    setFlag(N, (temp_ & 0x80) != 0);
    if (lookup_table_[opcode_].addrmode == &CPU::IMP) {
        reg_.a = temp_ & 0x00FF;
    }
    else {
        write(addr_abs_, temp_ & 0x00FF);
    }
    return 0;
}

// Description: Return from Interrupt
//              The status register is pulled with the break flag
//              and bit 5 ignored. Then PC is pulled from the stack.
// Function:    pull SR, pull PC
uint8_t CPU::RTI()
{
    reg_.st += 1;
    reg_.status = read(0x0100 + reg_.st);
    reg_.status &= ~B;
    reg_.status &= ~U;

    reg_.st += 1;
    reg_.pc = static_cast<uint16_t>(read(0x0100 + reg_.st));
    reg_.st += 1;
    reg_.pc |= static_cast<uint16_t>(read(0x0100 + reg_.st)) << 8;

    return 0;
}

// Description:     Return from Subroutine
// Function:        pull PC, PC+1 -> PC
uint8_t CPU::RTS()
{
    reg_.st += 1;
    reg_.pc = static_cast<uint16_t>(read(0x0100 + reg_.st));
    reg_.st += 1;
    reg_.pc |= static_cast<uint16_t>(read(0x0100 + reg_.st)) << 8;

    reg_.pc += 1;
    return 0;
}

// Description:     Subtract Memory from Accumulator with Borrow
// Function:        A - M - C̅ -> A
// Condition code:  N, Z, C, V
uint8_t CPU::SBC()
{
    fetch();

    // We can invert the bottom 8 bits with bitwise xor
    uint16_t value = (static_cast<uint16_t>(fetched_)) ^ 0x00FF;

    // Notice this is exactly the same as addition from here!
    temp_ = static_cast<uint16_t>(reg_.a) + value + static_cast<uint16_t>(getFlag(C));
    setFlag(C, (temp_ & 0xFF00) != 0);
    setFlag(Z, ((temp_ & 0x00FF) == 0));
    setFlag(V, (((static_cast<uint16_t>(reg_.a) & static_cast<uint16_t>(value)) ^ temp_) & 0x0080)
                   != 0);
    setFlag(N, (temp_ & 0x0080) != 0);
    reg_.a = temp_ & 0x00FF;
    return 1;
}

// Description:     Set Carry Flag
// Function:        1 -> C
// Condition code:  C(1)
uint8_t CPU::SEC()
{
    setFlag(C, true);
    return 0;
}

// Description:     Set Decimal Flag
// Function:        1 -> D
// Condition code:  D(1)
uint8_t CPU::SED()
{
    setFlag(D, true);
    return 0;
}

// Description:     Set Interrupt Disable Status
// Function:        1 -> I
// Condition code:  I(1)
uint8_t CPU::SEI()
{
    setFlag(I, true);
    return 0;
}

// Description: Store Accumulator in Memory
// Function:    A -> M
uint8_t CPU::STA()
{
    write(addr_abs_, reg_.a);
    return 0;
}

// Description: Store Index X in Memory
// Function:    X -> M
uint8_t CPU::STX()
{
    write(addr_abs_, reg_.x);
    return 0;
}

// Description: Sore Index Y in Memory
// Function:    Y -> M
uint8_t CPU::STY()
{
    write(addr_abs_, reg_.y);
    return 0;
}

// Description:     Transfer Accumulator to Index Y
// Function:        A -> X
// Condition code:  N, Z
uint8_t CPU::TAX()
{
    reg_.x = reg_.a;
    setFlag(Z, reg_.x == 0x00);
    setFlag(N, (reg_.x & 0x80) != 0);
    return 0;
}

// Description: transfer accumulator to Y index
// Function: A -> Y
// Condition code: N, Z
uint8_t CPU::TAY()
{
    reg_.y = reg_.a;
    setFlag(Z, reg_.y == 0x00);
    setFlag(N, (reg_.y & 0x80) != 0);
    return 0;
}

// Description:     Transfer Stack Pointer to Index X
// Function:        S -> X
// Condition code:  N, Z
uint8_t CPU::TSX()
{
    reg_.x = reg_.st;
    setFlag(Z, reg_.x == 0x00);
    setFlag(N, (reg_.x & 0x80) != 0);
    return 0;
}

// Description:     Transfer Index X to Accumulator
// Function:        X -> A
// Condition code:  N, Z
uint8_t CPU::TXA()
{
    reg_.a = reg_.x;
    setFlag(Z, reg_.a == 0x00);
    setFlag(N, (reg_.a & 0x80) != 0);
    return 0;
}

// Description: Transfer Index X to Stack Register
// Function:    X -> S
uint8_t CPU::TXS()
{
    reg_.st = reg_.x;
    return 0;
}

// Description:     Transfer Index Y to Accumulator
// Function:        Y -> A
// Condition code:  N, Z
uint8_t CPU::TYA()
{
    reg_.a = reg_.y;
    setFlag(Z, reg_.a == 0x00);
    setFlag(N, (reg_.a & 0x80) != 0);
    return 0;
}

// This function captures illegal opcodes
uint8_t CPU::XXX() { return 0; }

} // namespace tn