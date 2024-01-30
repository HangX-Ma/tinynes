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

    struct Sound
    {
        double sample{0.0};
        bool is_enable{false};
        Sequencer sequencer;
    };

    Sound pulse1_;
};

} // namespace tn

#endif