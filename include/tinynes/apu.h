#ifndef TINYNES_APU_H
#define TINYNES_APU_H

#include <cstdint>
#include <functional>

namespace tn
{

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

    // NES Dev wiki - APU Envelope: https://www.nesdev.org/wiki/APU_Envelope
    struct Envelope
    {
    };

    struct Sound
    {
        double sample{0.0};
        bool is_enable{false};
        Sequencer sequencer;
        OscillatorPulse osc;
    };

    Sound pulse1_;
    double global_time_{0.0};
};

} // namespace tn

#endif