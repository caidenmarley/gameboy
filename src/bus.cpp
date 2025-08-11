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

extern bool key_state[8];

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
        uint8_t res = 0xFF;

        if (!(joyp & 0x10)) {
            // action buttons selected → simulate Start (bit 3) is pressed
            res &= ~(1 << 3); // Start = 0 = pressed
        }

        // set upper bits to reflect joyp (bits 4 and 5)
        res = (res & 0xCF) | (joyp & 0x30);
        
        return res;
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
    else if(address == 0xFF00) {
        uint8_t res = 0xFF;

        if (!(joyp & 0x10)) {
            // action buttons selected simulate Start (bit 3) is pressed
            res &= ~(1 << 3); // Start = 0 = pressed
        }

        // set upper bits to reflect joyp (bits 4 and 5)
        res = (res & 0xCF) | (joyp & 0x30);
        
        return res;
    }
    else if(address < 0xFF04) return ioRegs[address - 0xFF00];
    else if(address < 0xFF08) return timer.read(address);
    else if(address >= 0xFF40 && address <= 0xFF4B) return ppu.read(address);
    else if(address < 0xFF80) return ioRegs[address - 0xFF00];
    else if(address < 0xFFFF) return hram[address - 0xFF80];
    else return ieReg;
}


void Bus::write(uint16_t address, uint8_t byte){
    if(address == 0xFF00){
        joyp = (byte & 0x30) | 0xCF; // only bit 4 and 5 are writable
        return;
    }

    if(ppu.isOamDmaActive() && (address < 0xFF80 || address > 0xFFFE)){
        // if OAM DMA is active cpu can only access HRAM
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
    else if(address < 0xFF00){} // unused
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