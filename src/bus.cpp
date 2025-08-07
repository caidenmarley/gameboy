#include "bus.h"
#include <stdexcept>
#include <algorithm>

Bus::Bus(Cartridge& cart) : cart(cart), timer(), ppu(*this){
    // clear all ram regions
    std::fill(std::begin(wram), std::end(wram), 0);
    std::fill(std::begin(hram), std::end(hram), 0);
    std::fill(std::begin(ioRegs), std::end(ioRegs), 0);
}

uint8_t Bus::read(uint16_t address){
    if(address < 0x8000){
        // Cartridge ROM
        return cart.readByte(address);
    }else if(address < 0xA000){
        // VRAM
        return ppu.read(address);
    }else if(address < 0xC000){
        // Cartridge RAM
        return cart.readByte(address);
    }else if(address < 0xE000){
        // WRAM
        return wram[address - 0xC000];
    }else if(address < 0xFE00){
        // WRAM, echo of first one
        return wram[address - 0xE000];
    }else if(address < 0xFEA0){
        // OAM
        return ppu.read(address);
    }else if(address < 0xFF00){
        // unused
        return 0xFF;
    }else if(address < 0xFF04){
        // IO regs
        return ioRegs[address - 0xFF00];
    }else if(address < 0xFF08){
        // timer regs
        return timer.read(address);
    }else if(address >= 0xFF40 && address <= 0xFF4B){
        // LCDC, STAT, etc
        return ppu.read(address);
    }else if(address < 0xFF80){
        // IO regs
        return ioRegs[address - 0xFF00];
    }else if(address < 0xFFFF){
        // HRAM
        return hram[address - 0xFF80];
    }else{
        // FFFF - interrupt enable reg
        return ieReg;
    }
}

void Bus::write(uint16_t address, uint8_t byte){
    if(address < 0x8000){
        // Cartridge ROM
        cart.writeByte(address, byte);
    }else if(address < 0xA000){
        // VRAM
        ppu.write(address, byte);
    }else if(address < 0xC000){
        // Cartridge RAM
        cart.writeByte(address, byte);
    }else if(address < 0xE000){
        // WRAM
        wram[address - 0xC000] = byte;
    }else if(address < 0xFE00){
        // WRAM
        wram[address - 0xE000] = byte;
    }else if(address < 0xFEA0){
        // OAM
        ppu.write(address, byte);
    }else if(address < 0xFF00){
        // unused
    }else if(address < 0xFF04){
        // IO regs, before timer
        ioRegs[address - 0xFF00] = byte;
    }else if(address < 0xFF08){
        // Timer regs
        timer.write(address, byte);
    }else if(address >= 0xFF40 && address <= 0xFF4B){
        // LCDC, STAT etc and DMA
        ppu.write(address, byte);
    }else if(address < 0xFF80){
        // IO regs, after timer
        ioRegs[address - 0xFF00] = byte;
    }else if(address < 0xFFFF){
        // HRAM
        hram[address - 0xFF80] = byte;
    }else{
        // FFFF - interrupt enable reg
        ieReg = byte;
    }
}

void Bus::step(int tStates, CPU& cpu){
    timer.step(tStates, cpu);
    ppu.step(tStates, cpu);
    // TODO
}