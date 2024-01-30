/**
 * @file demo_sound2.cpp
 * @brief real time music keyboard
 * @ref https://en.sfml-dev.org/forums/index.php?topic=24924.0
 * @version 0.1
 * @date 2024-01-30
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <cmath>
#include <string>
#include <vector>

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

int screen_width = 800;
int screen_height = 600;

#define MAXVOL (0.22 * 32768)
#define DECAY (MAXVOL / 4000)

#define KEYS 32
#define M_PI 3.14159265358979323846

float vol[KEYS] = {0};
float phase[KEYS] = {0};

int octave = 0;

const double PI2 = M_PI * 2;
const float BASE_A_FREQ = 440;
const int SAMPLE_RATE = 44100;
const int NUM_CHANNEL = 2;
const int AUDIO_BUF_SIZE = 2048;

// 0 sin
// 1 triangle
// 2 saw
// 3 square
// 4 white noise
int wave_shape_type = 0;
class ShapeMode
{
public:
    static int const SINE = 0;
    static int const TRIANGLE = 1;
    static int const SAW = 2;
    static int const SQUARE = 3;
    static int const WHITE_NOISE = 4;
};

sf::RenderWindow window;

std::string title_str;
std::string note_str = "0";
std::string oct_str = "0";
std::string wave_shape_str = "0";

bool key_buf[sf::Keyboard::Key::KeyCount] = {false};
bool key_buf_last[sf::Keyboard::Key::KeyCount] = {false};

int keymap[]{sf::Keyboard::Key::Z,        sf::Keyboard::Key::S,        sf::Keyboard::Key::X,
             sf::Keyboard::Key::D,        sf::Keyboard::Key::C,        sf::Keyboard::Key::V,
             sf::Keyboard::Key::G,        sf::Keyboard::Key::B,        sf::Keyboard::Key::H,
             sf::Keyboard::Key::N,        sf::Keyboard::Key::J,        sf::Keyboard::Key::M,
             sf::Keyboard::Key::Q,        sf::Keyboard::Key::Num2,     sf::Keyboard::Key::W,
             sf::Keyboard::Key::Num3,     sf::Keyboard::Key::E,        sf::Keyboard::Key::R,
             sf::Keyboard::Key::Num5,     sf::Keyboard::Key::T,        sf::Keyboard::Key::Num6,
             sf::Keyboard::Key::Y,        sf::Keyboard::Key::Num7,     sf::Keyboard::Key::U,
             sf::Keyboard::Key::I,        sf::Keyboard::Key::Num9,     sf::Keyboard::Key::O,
             sf::Keyboard::Key::Num0,     sf::Keyboard::Key::P,        sf::Keyboard::Key::LBracket,
             sf::Keyboard::Key::RBracket, sf::Keyboard::Key::Backslash};

int num_keys = sizeof(keymap) / sizeof(keymap[0]);

bool isKeyPressOnce(int keyCode) { return key_buf[keyCode] && !key_buf_last[keyCode]; }

void updateAllString()
{
    title_str = "waveShape: " + wave_shape_str + ", octave: " + oct_str + ", note: " + note_str;
    window.setTitle(title_str);
}

class MyStream : public sf::SoundStream
{
public:
    void setBufSize(int sz, int numChannel, int sampleRate)
    {
        samples_.resize(sz, 0);
        currentSample_ = 0;
        initialize(numChannel, sampleRate);
    }

private:
    bool onGetData(Chunk &data) override
    {
        int len = samples_.size();
        for (int i = 0; i < len; ++i) {
            samples_[i] = 0;
        }

        data.samples = &samples_[currentSample_];

        int k = 0;
        int c = 0;
        float s = 0;
        float increment = 0;

        for (k = 0; k < num_keys; k++) {
            if (vol[k] <= 0) {
                continue;
            }

            increment = PI2 * pow(2.0, (k + 3 + 12 * octave) / 12.0) * BASE_A_FREQ / SAMPLE_RATE;

            for (c = 0; c < AUDIO_BUF_SIZE; c += NUM_CHANNEL) {
                if (wave_shape_type == ShapeMode::SINE) {
                    s = samples_[c] + std::sin(phase[k]) * vol[k];
                }
                else if (wave_shape_type == ShapeMode::TRIANGLE) {
                    double t2 = fmodf(phase[k], PI2) / (PI2);
                    int t3 = floor(t2 + 0.5F);
                    double t4 = fabs(t2 - t3) * 4 - 1;
                    s = samples_[c] + t4 * vol[k];
                }
                else if (wave_shape_type == ShapeMode::SAW) {
                    double t2 = phase[k] / PI2;
                    int t3 = floor(t2 + 0.5F);
                    double t4 = (t2 - t3) * 2;
                    s = samples_[c] + t4 * vol[k];
                }
                else if (wave_shape_type == ShapeMode::SQUARE) {
                    double t2 = fmodf(phase[k], PI2) / (PI2);
                    double t3 = t2 > 0.5 ? 1 : -1;
                    s = samples_[c] + t3 * vol[k];
                }
                else if (wave_shape_type == ShapeMode::WHITE_NOISE) {
                    s = rand() % 65536 - 32768;
                }

                s = std::max(std::min(s, 32767.0F), -32767.0F);

                samples_[c] = s;
                samples_[c + 1] = s;

                phase[k] += increment;

                if (vol[k] < MAXVOL) {
                    vol[k] -= DECAY;
                    if (vol[k] <= 0) {
                        vol[k] = 0;
                        break;
                    }
                }
            }
            phase[k] = fmod(phase[k], PI2);
        }

        data.sampleCount = AUDIO_BUF_SIZE;
        currentSample_ = 0;

        return true;
    }

    void onSeek(sf::Time timeOffset) override
    {
        currentSample_ = static_cast<std::size_t>(timeOffset.asSeconds() * getSampleRate()
                                                  * getChannelCount());
    }

    std::vector<sf::Int16> samples_;
    std::size_t currentSample_;
};

int main()
{
    window.create(sf::VideoMode(screen_width, screen_height), "SFML2 Simple Audio Synth");
    window.setPosition(sf::Vector2i(sf::VideoMode::getDesktopMode().width / 4,
                                    sf::VideoMode::getDesktopMode().height / 4));

    MyStream stream;
    stream.setBufSize(AUDIO_BUF_SIZE, NUM_CHANNEL, SAMPLE_RATE);
    stream.play();

    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            switch (ev.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                key_buf[ev.key.code] = true;
                break;
            case sf::Event::KeyReleased:
                key_buf[ev.key.code] = false;
                break;
            default:
                break;
            }
        }

        if (key_buf[sf::Keyboard::Escape]) {
            window.close();
        }

        for (int key = 0; key < num_keys; ++key) {
            if (key_buf[keymap[key]] && vol[key] < MAXVOL) {
                phase[key] = 0;
                vol[key] = MAXVOL + DECAY / 2;

                int note = key + 12 * octave;

                note_str = std::to_string(note);
                updateAllString();
            }

            if (!key_buf[keymap[key]] && vol[key] > 0) {
                vol[key] -= DECAY;
            }
        }

        if (isKeyPressOnce(sf::Keyboard::Key::Left)) {
            --wave_shape_type;
            wave_shape_type = wave_shape_type % 4;

            wave_shape_str = std::to_string(wave_shape_type);
            updateAllString();
        }
        if (isKeyPressOnce(sf::Keyboard::Key::Right)) {
            ++wave_shape_type;
            wave_shape_type = wave_shape_type % 4;

            wave_shape_str = std::to_string(wave_shape_type);
            updateAllString();
        }

        if (isKeyPressOnce(sf::Keyboard::Key::Down)) {
            --octave;
            octave = std::max(octave, -3);

            oct_str = std::to_string(octave);
            updateAllString();
        }
        if (isKeyPressOnce(sf::Keyboard::Key::Up)) {
            ++octave;
            octave = std::min(octave, 4);

            oct_str = std::to_string(octave);
            updateAllString();
        }

        window.display();
        window.clear();

        sf::sleep(sf::microseconds(1));

        for (int k = 0; k < sf::Keyboard::Key::KeyCount; ++k) {
            key_buf_last[k] = key_buf[k];
        }
    }

    return 0;
}