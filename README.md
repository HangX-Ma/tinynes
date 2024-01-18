# tinynes

NES emulator based on 6052 CPU written from scratch.
> This project based on [OneLoneCoder/olcNES](https://github.com/OneLoneCoder/olcNES), following rules described in **OneLoneCoder**'s license.

## Prerequisite

- Ubuntu 20.04 or higher
- CMake 3.14 or higher
- Install SFML2.x for graphic debug.

```bash
sudo apt-get install build-essential
sudo apt-get install libsfml-dev
```

## Schedule

- [x] _**STEP 1**_: Complete 6052 processor instruction set and adapt SFML graphic library
- [ ] Add PPU(Picture Processing Unit) with other memory unit like ROM, Cartridge, etc

## NES Emulator Concept

<div class="dino" align="center">
    <img src="./assets/nes_structure.svg" alt="NES Memory Map" width=480 />
    <br>
    <font size="2" color="#999"><u>NES Memory Map</u></font>
</div>

You can find the description of CPU memory map from [NES Dev wiki: CPU memory map](https://www.nesdev.org/wiki/CPU_memory_map). The above picture used as a note to describe the NES emulator system.

All components are linked and managed by the _**main BUS**_. The bus takes the responsibility to transfer data and message. Except the _**main BUS**_, PPU has its own bus to communicates with other graphic related components such as _pattern tables_, _name tables_, _palettes_, etc.

## Reference

- [NES Dev wiki: NES reference guide](https://www.nesdev.org/wiki/NES_reference_guide)
- [OneLoneCoder/olcNES](https://github.com/OneLoneCoder/olcNES)
- [6502 Instruction Set](https://www.masswerk.at/6502/6502_instruction_set.html)
- [6502CPU以及NES游戏机系统](http://49.212.183.201/6502/6502_report.htm)

## License

MIT License
