#include "tinynes/gui.h"

int main()
{
    gui::GUI gui;
    gui.init(680, 480, "TinyNES");
    gui.loop();

    return 0;
}