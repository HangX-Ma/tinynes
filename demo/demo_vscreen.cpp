#include "tinynes/gui.h"
#include "tinynes/palette_color.h"
#include "tinynes/vscreen.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cstdlib>
#include <spdlog/spdlog.h>

void genSnowNoise(uint32_t width, uint32_t height, tn::VScreen &vs)
{
    if (width > vs.width() || height > vs.height()) {
        spdlog::warn("width {} or height {} violates screen bounding", width, height);
        return;
    }
    for (uint32_t row = 0; row < width; row += 1) {
        for (uint32_t col = 0; col < height; col += 1) {
            vs.setPixel(row, col, sf::Color(colors[(std::rand() % 2) != 0 ? 0x3F : 0x30]));
        }
    }
}

int main()
{
    const int width = 680;
    const int height = 480;

    sf::RenderWindow window;
    window.create(sf::VideoMode(width, height), "TinyNES");
    window.setVerticalSyncEnabled(true);
    window.setPosition(sf::Vector2i(sf::VideoMode::getDesktopMode().width / 4,
                                    sf::VideoMode::getDesktopMode().height / 4));

    tn::VScreen vscreen(width, height, gui::ONE_DARK.dark);

    sf::Sprite sprite;
    vscreen.update(sprite);
    sprite.setPosition(0, 0);
    window.draw(sprite);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Snow noise
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            while (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                ; // waiting for key release
            }
            genSnowNoise(vscreen.width() / 4, vscreen.height() / 4, vscreen);
            vscreen.update(sprite);
            sprite.setPosition(0, 0);
            window.draw(sprite);
            spdlog::info("Generate snow noise");
        }
        // flush screen

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
            while (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
                ; // waiting for key release
            }
            vscreen.fill(gui::ONE_DARK.dark);
            vscreen.update(sprite);
            sprite.setPosition(0, 0);
            window.draw(sprite);
            spdlog::info("Flush screen");
        }

        window.display();
    }

    return 0;
}