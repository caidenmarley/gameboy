#include "bus.h"
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>

Bus::Bus(Cartridge& cart) : ppu(*this), cart(cart), timer(){
    // clear all ram regions
    std::fill(std::begin(wram), std::end(wram), 0);
    std::fill(std::begin(hram), std::end(hram), 0);
    std::fill(std::begin(ioRegs), std::end(ioRegs), 0);
}

uint8_t Bus::read(uint16_t address){
    if(ppu.isOamDmaActive() && (address < 0xFF80 || address > 0xFFFE)){
        // if OAM DMA is active cpu can only access HRAM
        return 0xFF;
    }

    if(address == 0xFF0F) return interruptFlag;

    if(address < 0x8000) return cart.readByte(address); // Cartridge ROM
    else if(address < 0xA000) return ppu.read(address); // VRAM
    else if(address < 0xC000) return cart.readByte(address); // Cartridge RAM
    else if(address < 0xE000) return wram[address - 0xC000]; // WRAM
    else if(address < 0xFE00) return wram[address - 0xE000]; // WRAM, echo of first one
    else if(address < 0xFEA0) return ppu.read(address); // OAM
    else if(address < 0xFF00) return 0xFF; // unused
    else if(address == 0xFF00){
        // Active low register, (joyp init to 11001111)
        uint8_t reg = joyp;
        
        // D pad, bit 4 = 0
        if(!(joyp & 0x10)){
            if(keys[4]) reg &= ~(1 << 0); // Right pressed
            if(keys[5]) reg &= ~(1 << 1); // Left pressed
            if(keys[6]) reg &= ~(1 << 2); // Up pressed
            if(keys[7]) reg &= ~(1 << 3); // Down pressed
        }

        if(!(joyp & 0x20)){
            if(keys[0]) reg &= ~(1 << 0); // A pressed
            if(keys[1]) reg &= ~(1 << 1); // B pressed
            if(keys[2]) reg &= ~(1 << 2); // Select pressed
            if(keys[3]) reg &= ~(1 << 3); // Start pressed
        }

        return reg;
    }else if(address < 0xFF04) return ioRegs[address - 0xFF00]; // IO regs
    else if(address < 0xFF08) return timer.read(address); // timer regs
    else if(address >= 0xFF40 && address <= 0xFF4B) return ppu.read(address); // LCDC, STAT, etc
    else if(address < 0xFF80) return ioRegs[address - 0xFF00]; // IO regs
    else if(address < 0xFFFF) return hram[address - 0xFF80]; // HRAM
    else return ieReg; // FFFF - interrupt enable reg
}

uint8_t Bus::readDuringDMA(uint16_t address) {
    // exact copy of read but without the guard
    if(address == 0xFF0F) return interruptFlag;

    if(address < 0x8000) return cart.readByte(address);
    else if(address < 0xA000) return ppu.read(address);
    else if(address < 0xC000) return cart.readByte(address);
    else if(address < 0xE000) return wram[address - 0xC000];
    else if(address < 0xFE00) return wram[address - 0xE000];
    else if(address < 0xFEA0) return ppu.read(address);
    else if(address < 0xFF00) return 0xFF;
    else if(address == 0xFF00){
        // Active low register, (joyp init to 11001111)
        uint8_t reg = joyp;
        
        // D pad, bit 4 = 0
        if(!(joyp & 0x10)){
            if(keys[4]) reg &= ~(1 << 0); // Right pressed
            if(keys[5]) reg &= ~(1 << 1); // Left pressed
            if(keys[6]) reg &= ~(1 << 2); // Up pressed
            if(keys[7]) reg &= ~(1 << 3); // Down pressed
        }

        if(!(joyp & 0x20)){
            if(keys[0]) reg &= ~(1 << 0); // A pressed
            if(keys[1]) reg &= ~(1 << 1); // B pressed
            if(keys[2]) reg &= ~(1 << 2); // Select pressed
            if(keys[3]) reg &= ~(1 << 3); // Start pressed
        }

        return reg;
    }else if(address < 0xFF04) return ioRegs[address - 0xFF00];
    else if(address < 0xFF08) return timer.read(address);
    else if(address >= 0xFF40 && address <= 0xFF4B) return ppu.read(address);
    else if(address < 0xFF80) return ioRegs[address - 0xFF00];
    else if(address < 0xFFFF) return hram[address - 0xFF80];
    else return ieReg;
}

void Bus::write(uint16_t address, uint8_t byte){    
    if(ppu.isOamDmaActive() && (address < 0xFF80 || address > 0xFFFE)){
        // if OAM DMA is active cpu can only access HRAM
        return;
    }

    if(address == 0xFF00){
        joyp = (byte & 0x30) | 0xCF; // only bit 4 and 5 are writable
        return;
    }

    if(address == 0xFF0F){
        interruptFlag = byte;
        return;
    }

    if(address < 0x8000) cart.writeByte(address, byte); // Cartridge ROM
    else if(address < 0xA000) ppu.write(address, byte); // VRAM
    else if(address < 0xC000) cart.writeByte(address, byte); // Cartridge RAM
    else if(address < 0xE000) wram[address - 0xC000] = byte; // WRAM
    else if(address < 0xFE00) wram[address - 0xE000] = byte; // WRAM
    else if(address < 0xFEA0) ppu.write(address, byte); // OAM
    else if(address < 0xFF00) return; // unused
    else if(address < 0xFF04) ioRegs[address - 0xFF00] = byte; // IO regs, before timer
    else if(address < 0xFF08) timer.write(address, byte); // Timer regs
    else if(address >= 0xFF40 && address <= 0xFF4B) ppu.write(address, byte); // LCDC, STAT etc and DMA
    else if(address < 0xFF80) ioRegs[address - 0xFF00] = byte; // IO regs, after timer
    else if(address < 0xFFFF) hram[address - 0xFF80] = byte; // HRAM
    else ieReg = byte; // FFFF - interrupt enable reg
    
}

void Bus::step(int tStates, CPU& cpu){
    timer.step(tStates, cpu);
    ppu.step(tStates, cpu);
    // TODO
}

void Bus::setKeyState(const bool keyState[8]){
    bool requestInterrupt = false;
    for(int i = 0; i < 8; ++i){
        bool was = keys[i];
        bool now = keyState[i];
        if(!was && now) requestInterrupt = true; // rising edge
        keys[i] = now;
    }
    if(requestInterrupt){
        // bit 4 IF 
        interruptFlag |= 0x10;
    }
}