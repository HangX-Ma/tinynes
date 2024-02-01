#ifndef TINYNES_APU_H
#define TINYNES_APU_H

#include <cstdint>
#include <functional>

namespace tn
{

// The APU has five channels: two pulse wave generators, a triangle wave, noise, and a delta
// modulation channel for playing DPCM samples.
class APU
{
public:
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
    void clock();
    void reset();

    double getOutputSample();

private:
    uint32_t frame_clock_counter_{0};
    uint32_t clock_counter_{0};

private:
    // NES Dev wiki - APU Frame Counter: https://www.nesdev.org/wiki/APU_Frame_Counter
    // NES Dev wiki - APU Pulse, sequencer behavior: https://www.nesdev.org/wiki/APU_Pulse

    struct Sequencer
    {
        uint32_t sequence{0};
        uint32_t new_sequence{0};
        uint16_t timer{0};
        uint16_t reload{0};
        uint8_t output{0};

        uint8_t clock(bool is_enable, const std::function<void(uint32_t &s)> &doSomething)
        {
            if (is_enable) {
                timer -= 1;
                if (timer == 0xFFFF) {
                    timer = reload;
                    doSomething(sequence);
                    output = sequence & 0x00000001;
                }
            }
            return output;
        }
    };

    struct OscillatorPulse
    {
        double frequency{0.0};
        double duty_cycle{0.0};
        double amplitude{1.0};
        double pi{3.1415926535};
        double harmonics{20};

        double sample(double t)
        {
            double a = 0;
            double b = 0;
            double p = duty_cycle * 2.0 * pi;

            // accumulate sin() calculation speed
            auto approxsin = [](double t)
            {
                double j = t * 0.15915;
                j = j - static_cast<int>(j);
                return 20.785 * j * (j - 0.5) * (j - 1.0);
            };

            // use sine wave to generate pulse
            for (double n = 1; n < harmonics; n += 1) {
                double c = n * frequency * 2.0 * pi * t;
                a += -approxsin(c) / n;
                b += -approxsin(c - p * n) / n;
            }
            return (2.0 * amplitude / pi) * (a - b);
        }
    };

    // NES Dev wiki - APU Length Counter: https://www.nesdev.org/wiki/APU_Length_Counter
    struct LengthCounter
    {
        uint8_t counter = 0x00;
        uint8_t clock(bool is_enable, bool is_halt)
        {
            if (!is_enable) {
                counter = 0;
            }
            else if (counter > 0 && !is_halt) {
                counter -= 1;
            }
            return counter;
        }
    };

    // NES Dev wiki - APU Envelope: https://www.nesdev.org/wiki/APU_Envelope
    // The NES APU has an envelope generator that controls the volume in one of two ways:
    // - it can generate a decreasing saw envelope with optional looping
    // - it can generate a constant volume that a more sophisticated software envelope generator can
    //   manipulate.
    struct Envelope
    {
        bool is_start{false};
        bool is_enable{false};
        uint16_t decay_count{0};
        uint16_t divider_count{0};
        uint16_t constant_volume{0};
        uint16_t output{0};

        void clock(bool is_loop)
        {
            if (!is_start) {
                if (divider_count == 0) {
                    divider_count = constant_volume;

                    if (decay_count == 0) {
                        if (is_loop) {
                            decay_count = 15;
                        }
                    }
                    else {
                        decay_count -= 1;
                    }
                }
                else {
                    decay_count -= 1;
                }
            }
            else {
                is_start = false;
                decay_count = 15;
                divider_count = constant_volume;
            }

            output = is_enable ? decay_count : constant_volume;
        }
    };

    // NES Dev wiki - APU Sweep: https://www.nesdev.org/wiki/APU_Sweep
    // Each of the two NES APU pulse (square) wave channels generate a pulse wave with variable
    // duty.
    struct Sweep
    {
        bool is_mute{false};
        bool is_enable{false};
        bool is_negate{false};
        bool is_reload{false};
        uint8_t shifter{0x00};
        uint8_t timer{0x00};
        uint8_t period{0x00};
        uint16_t change_amount{0};

        void track(uint16_t &target)
        {
            if (is_enable) {
                change_amount = target >> shifter;
                is_mute = (target < 0x8) || (target > 0x7FF);
            }
        }

        bool clock(uint16_t &target, bool channel)
        {
            bool is_changed = false;
            if (target >= 8 && change_amount < 0x07FF) {
                if (timer == 0 && is_enable && shifter > 0 && !is_mute) {
                    target = is_negate ? target - change_amount - static_cast<int>(channel)
                                       : target + change_amount;
                    is_changed = true;
                }
            }
            if (timer == 0 || is_reload) {
                timer = period;
                is_reload = false;
            }
            else {
                timer -= 1;
            }
            is_mute = (target < 8) || (target > 0x7FF);

            return is_changed;
        }
    };

    struct Sound
    {
        double sample{0.0};
        double output{0.0};
        bool is_halt{false};
        bool is_enable{false};
        Sequencer sequencer;
        OscillatorPulse osc;
        Envelope envelope;
        LengthCounter lc;
        Sweep sweep;
    };

    Sound pulse1_;
    Sound pulse2_;
    Sound noise_;
    static uint8_t length_table_[];
    double global_time_{0.0};
    bool is_raw_mode_{false};
};

} // namespace tn

#endif