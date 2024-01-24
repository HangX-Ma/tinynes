#ifndef TINYNES_VSCREEN_H
#define TINYNES_VSCREEN_H

#include <SFML/Config.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
namespace tn
{

// How to draw pixels using SFML: <https://en.sfml-dev.org/forums/index.php?topic=27832.0>
class VScreen
{
public:
    explicit VScreen(uint32_t width, uint32_t height, sf::Color color)
    {
        texture_.create(width, height);
        image_.resize(width * height * 4);

        for (uint32_t row = 0; row < width; row += 1) {
            for (uint32_t col = 0; col < height; col += 1) {
                image_[(row + col * width) * 4 + 0] = color.r;
                image_[(row + col * width) * 4 + 1] = color.g;
                image_[(row + col * width) * 4 + 2] = color.b;
                image_[(row + col * width) * 4 + 3] = color.a;
            }
        }
    }

    void setPixel(uint32_t x, uint32_t y, sf::Color color)
    {
        uint32_t idx = (x + y * width()) * 4;
        if (idx >= image_.capacity()) {
            return;
        }
        image_[idx + 0] = color.r;
        image_[idx + 1] = color.g;
        image_[idx + 2] = color.b;
        image_[idx + 3] = color.a;
    }

    void fill(sf::Color color)
    {
        for (uint32_t row = 0; row < width(); row += 1) {
            for (uint32_t col = 0; col < height(); col += 1) {
                image_[(row + col * width()) * 4 + 0] = color.r;
                image_[(row + col * width()) * 4 + 1] = color.g;
                image_[(row + col * width()) * 4 + 2] = color.b;
                image_[(row + col * width()) * 4 + 3] = color.a;
            }
        }
    }

    void update(sf::Sprite &spr)
    {
        texture_.update(image_.data());
        spr.setTexture(texture_);
    }

    uint32_t width() const { return texture_.getSize().x; }
    uint32_t height() const { return texture_.getSize().y; }

private:
    sf::Texture texture_;
    std::vector<sf::Uint8> image_;
};

} // namespace tn

#endif