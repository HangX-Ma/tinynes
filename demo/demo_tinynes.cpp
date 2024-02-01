#include "tinynes/gui.h"
#include "tinynes/utils.h"
#include "tinynes/vscreen.h"
#include "tinynes/vsound.h"

#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

static int selected_palette{0};

void guiRenderGame(gui::GUI &gui, sf::Sprite &sprite, sf::Vector2u &wsize,
                   [[maybe_unused]] float elapsed_time)
{
    std::function<uint8_t(sf::Keyboard::Key, uint8_t, std::string_view)> checker_func
        = [&](auto key, auto val, std::string_view name) -> uint8_t
    {
        if (sf::Keyboard::isKeyPressed(key)) {
            spdlog::debug("press {}", name);
            return val;
        }
        return 0x00;
    };

    gui.window().clear(gui::ONE_DARK.dark);

    gui.nes()->controller()[0] = 0x00;
    gui.nes()->controller()[0] |= checker_func(sf::Keyboard::X, 0x80, "X");
    gui.nes()->controller()[0] |= checker_func(sf::Keyboard::Z, 0x40, "Z");
    gui.nes()->controller()[0] |= checker_func(sf::Keyboard::A, 0x20, "A");
    gui.nes()->controller()[0] |= checker_func(sf::Keyboard::S, 0x10, "S");
    gui.nes()->controller()[0] |= checker_func(sf::Keyboard::Up, 0x08, "Up Arrow");
    gui.nes()->controller()[0] |= checker_func(sf::Keyboard::Down, 0x04, "Down Arrow");
    gui.nes()->controller()[0] |= checker_func(sf::Keyboard::Left, 0x02, "Left Arrow");
    gui.nes()->controller()[0] |= checker_func(sf::Keyboard::Right, 0x01, "Right Arrow");

    // reset
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
        gui.waitKeyReleased(sf::Keyboard::R);
        gui.nes()->reset();
    }

    // palette selection
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::P)) {
        gui.waitKeyReleased(sf::Keyboard::P);
        selected_palette += 1;
        selected_palette &= 0x07;
    }

    // draw main screen
    gui.nes()->ppu().vScreenMain()->update(sprite);
    sprite.setPosition(0, 0);
    sprite.setScale(2.0, 2.0);
    gui.window().draw(sprite);

    gui.window().display();
}

void guiLogic(gui::GUI &gui)
{
    sf::Sprite sprite;
    sf::Clock clock;

    sf::Vector2u wsize = gui.window().getSize();
    while (gui.window().isOpen()) {
        sf::Event event;
        while (gui.window().pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                gui.window().close();
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                gui.window().close();
            }
        }
        // RENDER MAIN BEGIN
        auto elapsed = clock.restart();
        guiRenderGame(gui, sprite, wsize, elapsed.asSeconds());
        // RENDER MAIN END

        sf::sleep(sf::microseconds(10));
    }
}

int main()
{
    gui::GUI gui;

    gui.init(680, 480, "TinyNES");

    sf::Vector2u wsize = gui.window().getSize();
    gui.setCPUPosition(wsize.x * 0.64, wsize.y * 0.02);
    gui.setOAMPosition(wsize.x * 0.64, wsize.y * 0.25);

    gui.loadCartridge();

    tn::VSound stream;
    stream.init(512, 1, 44100, gui.nes());
    stream.play();

    guiLogic(gui);

    return 0;
}