#include "tinynes/gui.h"

void guiLogic(gui::GUI &gui)
{
    while (gui.window().isOpen()) {
        sf::Event event;
        while (gui.window().pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                gui.window().close();
            }
        }

        gui.window().clear(gui::ONE_DARK.dark);

        // run an instruction
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            // wait until the 'space' key released
            while (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                ;
            }
            do {
                gui.nes()->cpu().clock();
            }
            while (!gui.nes()->cpu().complete());
        }
        // reset
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
            // wait until the 'reset' key released
            while (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
                ;
            }
            gui.nes()->cpu().reset();
        }
        // interrupt
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::I)) {
            // wait until the 'interrupt' key released
            while (sf::Keyboard::isKeyPressed(sf::Keyboard::I)) {
                ;
            }
            gui.nes()->cpu().irq();
        }
        // non maskable interrupt
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::N)) {
            // wait until the 'non maskable interrupt' key released
            while (sf::Keyboard::isKeyPressed(sf::Keyboard::N)) {
                ;
            }
            gui.nes()->cpu().nmi();
        }
        gui.renderCPU();
        gui.renderRAM();
        gui.renderCode();
        gui.renderInfo();

        gui.window().display();
    }
}

int main()
{
    gui::GUI gui;

    gui.init(680, 480, "TinyNES");

    sf::Vector2u wsize = gui.window().getSize();
    gui.setCPUPosition(wsize.x * 0.7, wsize.y * 0.02);
    gui.setRAMTopPosition(wsize.x * 0.02, wsize.y * 0.02);
    gui.setRAMBottomPosition(wsize.x * 0.02, wsize.y * 0.5);
    gui.setCodePosition(wsize.x * 0.7, wsize.y * 0.25);

    gui.loadSimpleProgram();

    guiLogic(gui);

    return 0;
}