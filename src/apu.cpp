#include "tinynes/apu.h"
#include <spdlog/spdlog.h>

namespace tn
{
uint8_t APU::length_table_[] = {10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
                                12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

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
        pulse1_.is_halt = static_cast<bool>(data & 0x20);
        pulse1_.envelope.is_enable = !static_cast<bool>(data & 0x10);
        pulse1_.envelope.constant_volume = (data & 0x0F);
        break;
    // pulse channel 1 sweep setup
    case 0x4001:
        pulse1_.sweep.shifter = data & 0x07;
        pulse1_.sweep.is_negate = static_cast<bool>(data & 0x08);
        pulse1_.sweep.period = (data & 0x70) >> 4;
        pulse1_.sweep.is_enable = static_cast<bool>(data & 0x80);
        pulse1_.sweep.is_reload = true;
        break;
    // pulse 1 timer Low 8 bits
    case 0x4002:
        pulse1_.sequencer.reload = (pulse1_.sequencer.reload & 0xFF00) | data;
        break;
    // Pulse 1 length counter load and timer high 3 bits
    case 0x4003:
        pulse1_.sequencer.reload
            = static_cast<uint16_t>((data & 0x07)) << 8 | (pulse1_.sequencer.reload & 0x00FF);
        pulse1_.sequencer.timer = pulse1_.sequencer.reload;
        pulse1_.sequencer.sequence = pulse1_.sequencer.new_sequence;
        pulse1_.lc.counter = length_table_[(data & 0xF8) >> 3];
        pulse1_.envelope.is_start = true;
        break;
    case 0x4004:
        switch ((data & 0xC0) >> 6) {
        case 0x00:
            pulse2_.sequencer.new_sequence = 0b01000000;
            pulse2_.osc.duty_cycle = 0.125;
            break;
        case 0x01:
            pulse2_.sequencer.new_sequence = 0b01100000;
            pulse2_.osc.duty_cycle = 0.250;
            break;
        case 0x02:
            pulse2_.sequencer.new_sequence = 0b01111000;
            pulse2_.osc.duty_cycle = 0.500;
            break;
        case 0x03:
            pulse2_.sequencer.new_sequence = 0b10011111;
            pulse2_.osc.duty_cycle = 0.750;
            break;
        }
        pulse2_.sequencer.sequence = pulse1_.sequencer.new_sequence;
        pulse2_.is_halt = static_cast<bool>(data & 0x20);
        pulse2_.envelope.is_enable = !static_cast<bool>(data & 0x10);
        pulse2_.envelope.constant_volume = (data & 0x0F);
        break;
        break;
    // pulse channel 2 sweep setup
    case 0x4005:
        pulse2_.sweep.shifter = data & 0x07;
        pulse2_.sweep.is_negate = static_cast<bool>(data & 0x08);
        pulse2_.sweep.period = (data & 0x70) >> 4;
        pulse2_.sweep.is_enable = static_cast<bool>(data & 0x80);
        pulse2_.sweep.is_reload = true;
        break;
    // pulse 2 timer low 8 bits
    case 0x4006:
        pulse2_.sequencer.reload = (pulse2_.sequencer.reload & 0xFF00) | data;
        break;
    // pulse 2 length counter load and timer high 3 bits
    case 0x4007:
        pulse2_.sequencer.reload
            = static_cast<uint16_t>((data & 0x07)) << 8 | (pulse2_.sequencer.reload & 0x00FF);
        pulse2_.sequencer.timer = pulse2_.sequencer.reload;
        pulse2_.sequencer.sequence = pulse2_.sequencer.new_sequence;
        pulse2_.lc.counter = length_table_[(data & 0xF8) >> 3];
        pulse2_.envelope.is_start = true;
        break;
    case 0x4008:
        break;
    case 0x400C:
        noise_.envelope.constant_volume = (data & 0x0F);
        noise_.envelope.is_enable = !static_cast<bool>(data & 0x10);
        noise_.is_halt = static_cast<bool>(data & 0x20);
        break;
    case 0x400E:
        switch (data & 0x0F) {
        case 0x00:
            noise_.sequencer.reload = 0;
            break;
        case 0x01:
            noise_.sequencer.reload = 4;
            break;
        case 0x02:
            noise_.sequencer.reload = 8;
            break;
        case 0x03:
            noise_.sequencer.reload = 16;
            break;
        case 0x04:
            noise_.sequencer.reload = 32;
            break;
        case 0x05:
            noise_.sequencer.reload = 64;
            break;
        case 0x06:
            noise_.sequencer.reload = 96;
            break;
        case 0x07:
            noise_.sequencer.reload = 128;
            break;
        case 0x08:
            noise_.sequencer.reload = 160;
            break;
        case 0x09:
            noise_.sequencer.reload = 202;
            break;
        case 0x0A:
            noise_.sequencer.reload = 254;
            break;
        case 0x0B:
            noise_.sequencer.reload = 380;
            break;
        case 0x0C:
            noise_.sequencer.reload = 508;
            break;
        case 0x0D:
            noise_.sequencer.reload = 1016;
            break;
        case 0x0E:
            noise_.sequencer.reload = 2034;
            break;
        case 0x0F:
            noise_.sequencer.reload = 4068;
            break;
        }
        break;
    case 0x400F:
        pulse1_.envelope.is_start = true;
        pulse2_.envelope.is_start = true;
        noise_.envelope.is_start = true;
        noise_.lc.counter = length_table_[(data & 0xF8) >> 3];
        break;
    case 0x4015:
        pulse1_.is_enable = static_cast<bool>(data & 0x01);
        pulse2_.is_enable = static_cast<bool>(data & 0x02);
        noise_.is_enable = static_cast<bool>(data & 0x04);
        break;
    }
}

uint8_t APU::cpuRead([[maybe_unused]] uint16_t addr) { return 0x00; }

void APU::clock()
{
    bool reach_quarter_frame_clock = false;
    bool reach_half_frame_clock = false;

    global_time_ += 0.3333333 / 1789773.0;

    // The sequencer is clocked on every other CPU cycle, so 2 CPU cycles = 1 APU cycle.
    // 3 PPU cycles = 1 CPU cycles.
    if (clock_counter_ % 6 == 0) {
        frame_clock_counter_ += 1;

        // 4-Step Sequence Mode - Mode 0: 4-Step Sequence (bit 7 of $4017 clear):
        // https://www.nesdev.org/wiki/APU_Frame_Counter
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
            pulse1_.envelope.clock(pulse1_.is_halt);
            pulse2_.envelope.clock(pulse2_.is_halt);
            noise_.envelope.clock(noise_.is_halt);
        }

        // Half frame "beats" adjust the note length counter and
        // frequency sweep units
        if (reach_half_frame_clock) {
            pulse1_.lc.clock(pulse1_.is_enable, pulse1_.is_halt);
            pulse2_.lc.clock(pulse2_.is_enable, pulse2_.is_halt);
            noise_.lc.clock(noise_.is_enable, noise_.is_halt);
            pulse1_.sweep.clock(pulse1_.sequencer.reload, 0);
            pulse2_.sweep.clock(pulse1_.sequencer.reload, 1);
        }

        // update pulse1 channel
        pulse1_.sequencer.clock(pulse1_.is_enable,
                                [](uint32_t &s)
                                {
                                    // Shift right by 1 bit, wrapping around
                                    s = ((s & 0x0001) << 7) | ((s & 0x00FE) >> 1);
                                });
        // pulse1_.sample = static_cast<double>(pulse1_.sequencer.output);
        // f = fCPU / (16 × (t + 1)), fCPU; 1.789773MHz NTSC
        pulse1_.osc.frequency
            = 1789773.0 / (16.0 * static_cast<double>(pulse1_.sequencer.reload + 1));

        pulse1_.osc.amplitude = static_cast<double>(pulse1_.envelope.output - 1) / 16.0;
        pulse1_.sample = pulse1_.osc.sample(global_time_);
        if (pulse1_.lc.counter > 0 && pulse1_.sequencer.timer >= 8 && !pulse1_.sweep.is_mute
            && pulse1_.envelope.output > 2)
        {
            pulse1_.output += (pulse1_.sample - pulse1_.output) * 0.5;
        }
        else {
            pulse1_.output = 0;
        }

        // update pulse2 channel
        pulse2_.sequencer.clock(pulse2_.is_enable,
                                [](uint32_t &s)
                                {
                                    // Shift right by 1 bit, wrapping around
                                    s = ((s & 0x0001) << 7) | ((s & 0x00FE) >> 1);
                                });
        pulse2_.osc.frequency
            = 1789773.0 / (16.0 * static_cast<double>(pulse2_.sequencer.reload + 1));
        pulse2_.osc.amplitude = static_cast<double>(pulse2_.envelope.output - 1) / 16.0;
        pulse2_.sample = pulse2_.osc.sample(global_time_);

        if (pulse2_.lc.counter > 0 && pulse2_.sequencer.timer >= 8 && !pulse2_.sweep.is_mute
            && pulse2_.envelope.output > 2)
        {
            pulse2_.output += (pulse2_.sample - pulse2_.output) * 0.5;
        }
        else {
            pulse2_.output = 0;
        }

        // update noise
        noise_.sequencer.clock(noise_.is_enable,
                               [](uint32_t &s)
                               {
                                   s = (((s & 0x0001) ^ ((s & 0x0002) >> 1)) << 14)
                                       | ((s & 0x7FFF) >> 1);
                               });

        if (noise_.lc.counter > 0 && noise_.sequencer.timer >= 8) {
            noise_.output = static_cast<double>(noise_.sequencer.output)
                            * (static_cast<double>(noise_.envelope.output - 1) / 16.0);
        }

        if (!pulse1_.is_enable) {
            pulse1_.output = 0;
        }
        if (!pulse2_.is_enable) {
            pulse2_.output = 0;
        }
        if (!noise_.is_enable) {
            noise_.output = 0;
        }
    }
    // Frequency sweepers change at high frequency
    pulse1_.sweep.track(pulse1_.sequencer.reload);
    pulse2_.sweep.track(pulse2_.sequencer.reload);

    clock_counter_ += 1;
}

void APU::reset() {}

double APU::getOutputSample()
{
    if (is_raw_mode_) {
        return (pulse1_.sample - 0.5) * 0.5 + (pulse2_.sample - 0.5) * 0.5;
    }
    return ((1.0 * pulse1_.output) - 0.8) * 0.1 + ((1.0 * pulse2_.output) - 0.8) * 0.1
           + ((2.0 * (noise_.output - 0.5))) * 0.1;
}
} // namespace tn