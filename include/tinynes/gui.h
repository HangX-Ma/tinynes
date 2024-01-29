#ifndef TINYNES_GUI_H
#define TINYNES_GUI_H

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <algorithm>
#include <memory>

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

#include "tinynes/cartridge.h"
#include "tinynes/cpu.h"
#include "tinynes/bus.h"
#include "tinynes/ppu.h"
#include "tinynes/utils.h"

namespace gui
{

struct OneDark
{
    sf::Color purple{198, 120, 221};
    sf::Color red{224, 108, 117};
    sf::Color yellow{229, 192, 123};
    sf::Color green{152, 195, 121};
    sf::Color cyan{86, 182, 194};
    sf::Color blue{97, 175, 239};
    sf::Color light_gray{171, 178, 191};
    sf::Color gray{92, 99, 112};
    sf::Color dark_gray{50, 54, 62};
    sf::Color dark{40, 44, 52};
};
static const OneDark ONE_DARK;
static const std::string ROOT_DIR = TINYNES_WORKSPACE;

class GUIUtils
{
public:
    static void setString(int x, int y, const std::string &s, sf::Color color, sf::Text &txt_board,
                          const sf::Font &font)
    {
        txt_board.setFont(font);
        txt_board.setFillColor(color);
        txt_board.setString(s);
        txt_board.setPosition(x, y);
        txt_board.setCharacterSize(15);
    }
};

class CPU
{
public:
    CPU()
    {
        if (!font_.loadFromFile("assets/UbuntuMono-Regular.ttf")) {
            spdlog::error("GUI-CPU load font failed!");
        }
        txt_status_.setCharacterSize(20);
    }

    void update(int x, int y, sf::Vector2u wsize, const tn::CPU &cpu)
    {
        int hspace = std::max(wsize.x / 100, static_cast<unsigned int>(1)); // pixel
        int vspace = std::max(wsize.y / 80, static_cast<unsigned int>(1));  // pixel
        GUIUtils::setString(x, y, "STATUS:", sf::Color::White, txt_status_, font_);
        GUIUtils::setString(
            txt_status_.getPosition().x + txt_status_.getLocalBounds().width + hspace, y, "N",
            cpu.checkFlag(tn::CPU::N) ? ONE_DARK.green : ONE_DARK.red, txt_flagN_, font_);
        GUIUtils::setString(txt_flagN_.getPosition().x + txt_flagN_.getLocalBounds().width + hspace,
                            y, "V", cpu.checkFlag(tn::CPU::V) ? ONE_DARK.green : ONE_DARK.red,
                            txt_flagV_, font_);
        GUIUtils::setString(txt_flagV_.getPosition().x + txt_flagV_.getLocalBounds().width + hspace,
                            y, "U", cpu.checkFlag(tn::CPU::U) ? ONE_DARK.green : ONE_DARK.red,
                            txt_flagU_, font_);
        GUIUtils::setString(txt_flagU_.getPosition().x + txt_flagU_.getLocalBounds().width + hspace,
                            y, "B", cpu.checkFlag(tn::CPU::B) ? ONE_DARK.green : ONE_DARK.red,
                            txt_flagB_, font_);
        GUIUtils::setString(txt_flagB_.getPosition().x + txt_flagB_.getLocalBounds().width + hspace,
                            y, "D", cpu.checkFlag(tn::CPU::D) ? ONE_DARK.green : ONE_DARK.red,
                            txt_flagD_, font_);
        GUIUtils::setString(txt_flagD_.getPosition().x + txt_flagD_.getLocalBounds().width + hspace,
                            y, "I", cpu.checkFlag(tn::CPU::I) ? ONE_DARK.green : ONE_DARK.red,
                            txt_flagI_, font_);
        GUIUtils::setString(txt_flagI_.getPosition().x + txt_flagI_.getLocalBounds().width + hspace,
                            y, "Z", cpu.checkFlag(tn::CPU::Z) ? ONE_DARK.green : ONE_DARK.red,
                            txt_flagZ_, font_);
        GUIUtils::setString(txt_flagZ_.getPosition().x + txt_flagZ_.getLocalBounds().width + hspace,
                            y, "C", cpu.checkFlag(tn::CPU::C) ? ONE_DARK.green : ONE_DARK.red,
                            txt_flagC_, font_);

        GUIUtils::setString(
            x, txt_status_.getPosition().y + txt_status_.getLocalBounds().height + 1.5 * vspace,
            "PC: $" + tn::Utils::numToHex(cpu.pc(), 4), sf::Color::White, txt_reg_pc_, font_);
        GUIUtils::setString(
            x, txt_reg_pc_.getPosition().y + txt_reg_pc_.getLocalBounds().height + vspace,
            "A:  $" + tn::Utils::numToHex(cpu.a(), 2) + "[" + std::to_string(cpu.a()) + "]",
            sf::Color::White, txt_reg_a_, font_);
        GUIUtils::setString(
            x, txt_reg_a_.getPosition().y + txt_reg_a_.getLocalBounds().height + vspace,
            "X:  $" + tn::Utils::numToHex(cpu.x(), 2) + "[" + std::to_string(cpu.x()) + "]",
            sf::Color::White, txt_reg_x_, font_);
        GUIUtils::setString(
            x, txt_reg_x_.getPosition().y + txt_reg_x_.getLocalBounds().height + vspace,
            "Y:  $" + tn::Utils::numToHex(cpu.y(), 2) + "[" + std::to_string(cpu.y()) + "]",
            sf::Color::White, txt_reg_y_, font_);
        GUIUtils::setString(
            x, txt_reg_y_.getPosition().y + txt_reg_y_.getLocalBounds().height + vspace,
            "S:  $" + tn::Utils::numToHex(cpu.st(), 4), sf::Color::White, txt_reg_st_, font_);
    }

    void drawTo(sf::RenderWindow &window)
    {
        window.draw(txt_status_);
        window.draw(txt_flagN_);
        window.draw(txt_flagV_);
        window.draw(txt_flagU_);
        window.draw(txt_flagB_);
        window.draw(txt_flagD_);
        window.draw(txt_flagI_);
        window.draw(txt_flagZ_);
        window.draw(txt_flagC_);
        window.draw(txt_reg_pc_);
        window.draw(txt_reg_a_);
        window.draw(txt_reg_x_);
        window.draw(txt_reg_y_);
        window.draw(txt_reg_st_);
    }

private:
    sf::Font font_;
    sf::Text txt_status_;
    sf::Text txt_flagN_;
    sf::Text txt_flagV_;
    sf::Text txt_flagU_;
    sf::Text txt_flagB_;
    sf::Text txt_flagD_;
    sf::Text txt_flagI_;
    sf::Text txt_flagZ_;
    sf::Text txt_flagC_;
    sf::Text txt_reg_pc_;
    sf::Text txt_reg_a_;
    sf::Text txt_reg_x_;
    sf::Text txt_reg_y_;
    sf::Text txt_reg_st_;
};

class RAM
{
public:
    RAM()
    {
        if (!font_.loadFromFile("assets/UbuntuMono-Regular.ttf")) {
            spdlog::error("GUI-RAM load font failed!");
        }
    }

    void update(int x, int y, sf::Vector2u wsize, std::unique_ptr<tn::Bus> &nes, uint16_t addr,
                int row_num, int col_num, sf::RenderWindow &window)
    {
        uint16_t offset;
        for (int row = 0; row < row_num; row += 1) {
            std::string txt = fmt::format("${}:", tn::Utils::numToHex(addr, 4));
            GUIUtils::setString(x, y, txt, ONE_DARK.yellow, txt_head_, font_);
            drawTo(window, txt_head_);
            txt.clear();

            offset = txt_head_.getLocalBounds().width;

            for (int col = 0; col < col_num; col += 1) {
                txt.append(" ");
                txt.append(tn::Utils::numToHex(nes->cpuRead(addr, true), 2));
                addr += 1;
            }
            GUIUtils::setString(x + offset, y, txt, sf::Color::White, txt_content_, font_);
            y += wsize.y * 0.03;
            drawTo(window, txt_content_);
        }
    }

private:
    void drawTo(sf::RenderWindow &window, sf::Text &txt_board) { window.draw(txt_board); }

private:
    sf::Text txt_head_;
    sf::Text txt_content_;
    sf::Font font_;
};

class Code
{

public:
    Code()
    {
        if (!font_.loadFromFile("assets/UbuntuMono-Regular.ttf")) {
            spdlog::error("GUI-RAM load font failed!");
        }
    }

    void update(int x, int y, int line_num, sf::Vector2u wsize, const tn::CPU::ASMMap &asm_map,
                const std::unique_ptr<tn::Bus> &nes, sf::RenderWindow &window)
    {
        int vspace = wsize.y * 0.03;

        // draw lower part
        int ny = (line_num >> 1) * vspace + y; // locates to middle of the code part
        auto iter = asm_map.find(nes->cpu().pc());
        // spdlog::info("pc: {}", nes->cpu().pc());
        if (iter != asm_map.end()) {
            GUIUtils::setString(x, ny, iter->second, ONE_DARK.blue, content_, font_);
            drawTo(window);
        }
        while (ny < line_num * vspace + y) {
            ny += vspace;
            if (++iter != asm_map.end()) {
                GUIUtils::setString(x, ny, iter->second, ONE_DARK.light_gray, content_, font_);
                drawTo(window);
            }
        }

        // draw current instruction according to PC
        iter = asm_map.find(nes->cpu().pc());
        ny = (line_num >> 1) * vspace + y;
        if (iter != asm_map.end()) {
            while (ny > y) {
                ny -= vspace;
                if (--iter != asm_map.end()) {
                    GUIUtils::setString(x, ny, iter->second, ONE_DARK.light_gray, content_, font_);
                    drawTo(window);
                }
            }
        }
    }

private:
    void drawTo(sf::RenderWindow &window) { window.draw(content_); }

private:
    sf::Text content_;
    sf::Font font_;
};

class GUI
{
public:
    void init(int width, int height, const std::string &title)
    {
        if (!default_font_.loadFromFile("assets/UbuntuMono-Regular.ttf")) {
            spdlog::error("GUI default font failed!");
        }

        window_.create(sf::VideoMode(width, height), title);
        window_.setVerticalSyncEnabled(true);
        window_.setPosition(sf::Vector2i(sf::VideoMode::getDesktopMode().width / 4,
                                         sf::VideoMode::getDesktopMode().height / 4));

        nes_ = std::make_unique<tn::Bus>();
    }

    void loadSimpleProgram()
    {
        uint16_t addr = 0x8000;
        std::stringstream ss;
        // Load Program (assembled at https://www.masswerk.at/6502/assembler.html)
        ss << "A2 0A 8E 00 00 A2 03 8E 01 00 AC 00 00 A9 00 18 6D 01 00 88 D0 FA 8D 02 00 EA EA EA";
        while (!ss.eof()) {
            std::string byte;
            ss >> byte;
            nes_->cpuRAM()[addr] = std::stoul(byte, nullptr, 16);
            addr += 1;
        }

        // Reset vector
        nes_->cpuRAM()[tn::RESET_VECTOR] = 0x00;
        nes_->cpuRAM()[tn::RESET_VECTOR + 1] = 0x80;

        // IRQ vector
        nes_->cpu().disassemble(0x0000, 0xFFFF, asm_map_);
        nes_->cpu().reset();
    }

    void loadCartridge()
    {
        std::string file_path = ROOT_DIR + "/test/nesfiles/donkey_kong.nes";
        cart_ = std::make_shared<tn::Cartridge>(file_path);
        if (!cart_->isNesFileLoaded()) {
            spdlog::error("{} complains it cannot load {}", __func__, file_path);
            return;
        }
        spdlog::info("load nes file: {}", file_path);

        nes_->insertCartridge(cart_);
        nes_->cpu().disassemble(0x0000, 0xFFFF, asm_map_);
        nes_->reset();
    }

    void setCPUPosition(uint x, uint y) { module_pos_.cpu = {x, y}; }
    void setRAMTopPosition(uint x, uint y) { module_pos_.ram_top = {x, y}; }
    void setRAMBottomPosition(uint x, uint y) { module_pos_.ram_bottom = {x, y}; }
    void setCodePosition(uint x, uint y) { module_pos_.code = {x, y}; }

    auto &window() { return window_; }
    auto &nes() { return nes_; }

    void waitKeyReleased(sf::Keyboard::Key key)
    {
        while (sf::Keyboard::isKeyPressed(key)) {
        }
    }

    void renderCPU()
    {
        gui_cpu_.update(module_pos_.cpu.x, module_pos_.cpu.y, window_.getSize(), nes_->cpu());
        gui_cpu_.drawTo(window_);
    }

    void renderRAM()
    {
        gui_ram_.update(module_pos_.ram_top.x, module_pos_.ram_top.y, window_.getSize(), nes_,
                        0x0000, 16, 16, window_);
        gui_ram_.update(module_pos_.ram_bottom.x, module_pos_.ram_bottom.y, window_.getSize(), nes_,
                        0x8000, 16, 16, window_);
    }

    void renderCode()
    {
        gui_code_.update(module_pos_.code.x, module_pos_.code.y, code_line_, window_.getSize(),
                         asm_map_, nes_, window_);
    }

    void renderInfo()
    {
        int code_vspace = window_.getSize().y * 0.03;
        int base_line_num = code_line_ + 1;
        // STEP
        GUIUtils::setString(
            module_pos_.code.x, module_pos_.code.y + (base_line_num + 1) * code_vspace,
            "SPACE = Step Instruction", ONE_DARK.purple, default_txt_board_, default_font_);

        window_.draw(default_txt_board_);
        // RESET
        GUIUtils::setString(module_pos_.code.x,
                            module_pos_.code.y + (base_line_num + 2) * code_vspace, "R = RESET",
                            ONE_DARK.purple, default_txt_board_, default_font_);
        window_.draw(default_txt_board_);
        // IRQ
        GUIUtils::setString(module_pos_.code.x,
                            module_pos_.code.y + (base_line_num + 3) * code_vspace, "I = IRQ",
                            ONE_DARK.purple, default_txt_board_, default_font_);
        window_.draw(default_txt_board_);
        // NMI
        GUIUtils::setString(module_pos_.code.x,
                            module_pos_.code.y + (base_line_num + 4) * code_vspace, "N = NMI",
                            ONE_DARK.purple, default_txt_board_, default_font_);
        window_.draw(default_txt_board_);
    }

private:
    sf::RenderWindow window_;
    std::shared_ptr<tn::Cartridge> cart_;
    std::unique_ptr<tn::Bus> nes_;
    tn::CPU::ASMMap asm_map_;

private:
    struct ModulePosition
    {
        sf::Vector2u cpu;
        sf::Vector2u ram_top;
        sf::Vector2u ram_bottom;
        sf::Vector2u code;
    } module_pos_;

    gui::CPU gui_cpu_;
    gui::RAM gui_ram_;
    gui::Code gui_code_;
    uint8_t code_line_{18};

    sf::Font default_font_;
    sf::Text default_txt_board_;
};

} // namespace gui

#endif