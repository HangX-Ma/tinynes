/**
 * @file demo_sound.cpp
 * @author contour.9x@gmail.com
 * @brief manual sound effect imitation
 * @ref https://github.com/SFML/SFML/wiki/Tutorial:-Play-Sine-Wave
 * @version 0.1
 * @date 2024-01-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <SFML/Audio.hpp>
#include <cmath>
#include <iostream>

const unsigned SAMPLES = 44100;
const unsigned SAMPLE_RATE = 44100;
const unsigned AMPLITUDE = 30000;

const double TWO_PI = 6.28318;

int main()
{
    sf::Int16 raw[SAMPLES];
    const double increment = 440. / 44100;
    double x = 0;
    for (int16_t &i : raw) {
        i = AMPLITUDE * sin(x * TWO_PI);
        x += increment;
    }

    sf::SoundBuffer buffer;
    if (!buffer.loadFromSamples(raw, SAMPLES, 1, SAMPLE_RATE)) {
        std::cerr << "Loading failed!" << std::endl;
        return 1;
    }

    sf::Sound sound;
    sound.setBuffer(buffer);
    sound.setLoop(true);
    sound.play();
    while (true) {
        sf::sleep(sf::milliseconds(100));
    }
    return 0;
}