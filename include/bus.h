#pragma once

#include <cstdint>
#include "cartridge.h"

class Bus{
public:
    Bus(Cartridge& cart);

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t byte);
private:
    Cartridge& cart;

    // 8 KiB video ram
    uint8_t vram[0x2000];
    // 8 KiB work ram
    uint8_t wram[0x2000];
    // sprite attribute memory OAM
    uint8_t oam[0xA0];
    // High ram (127 bytes)
    uint8_t hram[0x7F];
    // IO registers
    uint8_t ioRegs[0x80];
    // Interrupt enable
    uint8_t ieReg = 0;
};