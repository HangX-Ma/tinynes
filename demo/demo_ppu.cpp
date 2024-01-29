#include "tinynes/gui.h"
#include "tinynes/vscreen.h"
#include <SFML/Window/Keyboard.hpp>

static bool is_emulation_run{false};
static int selected_palette{0};

void guiLogic(gui::GUI &gui)
{
    sf::Sprite sprite;
    sf::Clock clock;

    sf::Vector2u wsize = gui.window().getSize();

    std::function<uint8_t(sf::Keyboard::Key, uint8_t, std::string_view)> checker_func
        = [&](auto key, auto val, std::string_view name) -> uint8_t
    {
        if (sf::Keyboard::isKeyPressed(key)) {
            spdlog::debug("press {}", name);
            return val;
        }
        return 0x00;
    };

    clock.restart();
    while (gui.window().isOpen()) {
        sf::Event event;
        while (gui.window().pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                gui.window().close();
            }

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

            if (is_emulation_run) {
                if (clock.getElapsedTime().asMilliseconds() > 1.0 / 300.0) {
                    clock.restart();
                    // limit the FPS to 60
                    do {
                        gui.nes()->clock();
                    }
                    while (!gui.nes()->ppu().getFrameState());
                    gui.nes()->ppu().setFrameState(false);
                }
            }
            else {
                // emulate one step
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
                    gui.waitKeyReleased(sf::Keyboard::C);
                    // Clock enough times to execute a whole CPU instruction
                    do {
                        gui.nes()->clock();
                    }
                    while (!gui.nes()->cpu().complete());

                    // CPU clock runs slower than system clock, so it may be
                    // complete for additional system clock cycles.
                    do {
                        gui.nes()->clock();
                    }
                    while (gui.nes()->cpu().complete());
                }

                // emulate one whole frame
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
                    gui.waitKeyReleased(sf::Keyboard::F);
                    // Clock enough times to draw a single frame
                    do {
                        gui.nes()->clock();
                    }
                    while (!gui.nes()->ppu().getFrameState());
                    // Use residual clock cycles to complete current instruction
                    do {
                        gui.nes()->clock();
                    }
                    while (!gui.nes()->cpu().complete());
                    // Reset frame completion flag
                    gui.nes()->ppu().setFrameState(false);
                }
            }

            // auto emulation
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                gui.waitKeyReleased(sf::Keyboard::Space);
                clock.restart();
                is_emulation_run = !is_emulation_run;
            }

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

            gui.renderCPU();
            gui.renderCode();

            // draw main screen
            gui.nes()->ppu().vScreenMain()->update(sprite);
            sprite.setPosition(0, 0);
            sprite.setScale(1.5, 1.5);
            gui.window().draw(sprite);

            // draw palette
            gui.nes()->ppu().vScreenPatternTable(0, selected_palette)->update(sprite);
            sprite.setPosition(wsize.x * 0.02, wsize.y * 0.75);
            sprite.setScale(0.5, 0.5);
            gui.window().draw(sprite);

            gui.nes()->ppu().vScreenPatternTable(1, selected_palette)->update(sprite);
            sprite.setPosition(wsize.x * 0.3, wsize.y * 0.75);
            sprite.setScale(0.5, 0.5);
            gui.window().draw(sprite);

            gui.window().display();
        }
    }
}

int main()
{
    gui::GUI gui;

    gui.init(680, 480, "TinyNES");

    sf::Vector2u wsize = gui.window().getSize();
    gui.setCPUPosition(wsize.x * 0.64, wsize.y * 0.02);
    gui.setCodePosition(wsize.x * 0.64, wsize.y * 0.25);
    gui.loadCartridge();

    guiLogic(gui);

    return 0;
}