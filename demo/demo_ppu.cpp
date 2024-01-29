#include "tinynes/gui.h"
#include "tinynes/vscreen.h"

static bool is_emulation_run{false};
static float residual_time{0.0F};
static int selected_palette{0};

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

            gui.window().clear(gui::ONE_DARK.dark);

            if (is_emulation_run) {
                float elapsed = clock.restart().asSeconds();
                if (residual_time > 0.0F) {
                    residual_time -= elapsed;
                }
                else {
                    residual_time += (1.0F / 60.0F);
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

            // draw palette
            gui.nes()->ppu().vScreenPatternTable(0, selected_palette)->update(sprite);
            sprite.setPosition(wsize.x * 0.02, wsize.y * 0.78);
            sprite.setScale(0.4, 0.4);
            gui.window().draw(sprite);

            gui.nes()->ppu().vScreenPatternTable(1, selected_palette)->update(sprite);
            sprite.setPosition(wsize.x * 0.2, wsize.y * 0.78);
            sprite.setScale(0.4, 0.4);
            gui.window().draw(sprite);

            // draw main screen
            gui.nes()->ppu().vScreenMain()->update(sprite);
            sprite.setPosition(0, 0);
            sprite.setScale(1.5, 1.5);
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