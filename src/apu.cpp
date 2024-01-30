#include "tinynes/apu.h"
#include <spdlog/spdlog.h>

namespace tn
{
void APU::cpuWrite(uint16_t addr, uint8_t data)
{
    // NES Dev wiki - APU: https://www.nesdev.org/wiki/APU
    switch (addr) {
    case 0x4000:
        // NES Dev wiki - APU pulse: https://www.nesdev.org/wiki/APU_Pulse
        // look at Sequencer behavior: Duty Cycle Sequences
        // The pulse channels produce a variable-width pulse signal, controlled by volume, envelope,
        // length, and sweep units.
        // [$4000/$4004] [DDLC VVVV]    Duty (D), envelope loop / length counter halt (L),
        //                              constant volume (C), volume/envelope (V)
        // [$4001/$4005] [EPPP NSSS]    Sweep unit: enabled (E), period (P), negate (N), shift (S)
        // [$4002/$4006] [TTTT TTTT]    Timer low (T)
        // [$4003/$4007] [TTTT LTTT]    Length counter load (L), timer high (T)
        switch ((data & 0xC0) >> 6) {
        case 0x00:
            pulse1_.sequencer.new_sequence = 0b01000000;
            pulse1_.osc.duty_cycle = 0.125;
            break;
        case 0x01:
            pulse1_.sequencer.new_sequence = 0b01100000;
            pulse1_.osc.duty_cycle = 0.250;
            break;
        case 0x02:
            pulse1_.sequencer.new_sequence = 0b01111000;
            pulse1_.osc.duty_cycle = 0.500;
            break;
        case 0x03:
            pulse1_.sequencer.new_sequence = 0b10011111;
            pulse1_.osc.duty_cycle = 0.750;
            break;
        }
        pulse1_.sequencer.sequence = pulse1_.sequencer.new_sequence;

        break;
    case 0x4001:
        break;
    case 0x4002:
        pulse1_.sequencer.reload = (pulse1_.sequencer.reload & 0xFF00) | data;
        break;
    case 0x4003:
        pulse1_.sequencer.reload
            = static_cast<uint16_t>((data & 0x07)) << 8 | (pulse1_.sequencer.reload & 0x00FF);
        pulse1_.sequencer.timer = pulse1_.sequencer.reload;
        pulse1_.sequencer.sequence = pulse1_.sequencer.new_sequence;
        break;
    case 0x4004:
        break;
    case 0x4005:
        break;
    case 0x4006:
        break;
    case 0x4007:
        break;
    case 0x4008:
        break;
    case 0x400C:
        break;
    case 0x400E:
        break;
    case 0x400F:
        break;
    case 0x4015:
        pulse1_.is_enable = static_cast<bool>(data & 0x01);
        break;
    }
}

uint8_t APU::cpuRead(uint16_t addr) { return 0x00; }

void APU::clock()
{
    bool reach_quarter_frame_clock = false;
    bool reach_half_frame_clock = false;

    global_time_ += 0.3333333 / 1789773.0;

    if (clock_counter_ % 6 == 0) {
        frame_clock_counter_ += 1;

        // 4-Step Sequence Mode
        if (frame_clock_counter_ == 3729) {
            reach_quarter_frame_clock = true;
        }

        if (frame_clock_counter_ == 7457) {
            reach_quarter_frame_clock = true;
            reach_half_frame_clock = true;
        }

        if (frame_clock_counter_ == 11186) {
            reach_quarter_frame_clock = true;
        }

        if (frame_clock_counter_ == 14916) {
            reach_quarter_frame_clock = true;
            reach_half_frame_clock = true;
            frame_clock_counter_ = 0;
        }
        // quarter frame "beats" adjust the volume envelope
        if (reach_quarter_frame_clock) {
        }

        // Half frame "beats" adjust the note length and
        // frequency sweepers
        if (reach_half_frame_clock) {
        }

        // update pulse1 channel
        // pulse1_.sequencer.clock(pulse1_.is_enable,
        //                         [](uint32_t &s)
        //                         {
        //                             // Shift right by 1 bit, wrapping around
        //                             s = ((s & 0x0001) << 7) | ((s & 0x00FE) >> 1);
        //                         });
        // pulse1_.sample = static_cast<double>(pulse1_.sequencer.output);
        // f = fCPU / (16 × (t + 1)), fCPU; 1.789773MHz NTSC
        pulse1_.osc.frequency
            = 1789773.0 / (16.0 * static_cast<double>(pulse1_.sequencer.reload + 1));
        pulse1_.sample = pulse1_.osc.sample(global_time_);
    }
    clock_counter_ += 1;
}

void APU::reset() {}

double APU::getOutputSample() { return pulse1_.sample; }

} // namespace tn