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
    }else if(address == 0xFF00){
        uint8_t res = joyp;

        // if bit 4 is clear then buttons row (A,B,Select,Start)
        if(!(joyp & 0x10)){
            for(int i = 0; i < 4; ++i){
                if(key_state[i]){   // key_state[0]=A, 1=B, 2=Select, 3=Start
                    res &= ~(1 << i);
                }
            }
        }else if(!(joyp & 0x20)){ // if bit 5 clear, directions row (right, left, up, down)
            for(int i = 0; i < 4; ++i){
                if(key_state[i + 4]){ // key_state[4]=R, 5=L, 6=U, 7=D
                    res &= ~(1 << i);
                }
            }
        }

        return res;
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
    if(address == 0xFF00){
        joyp = (byte & 0x30) | 0xCF; // only bit 4 and 5 are writable
        return;
    }
    if(ppu.isOamDmaActive() && (address < 0xFF80 || address > 0xFFFE)){
        // if OAM DMA is active cpu can only access HRAM
        return;
    }

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
        std::cout << "here2" << std::endl;
        if(address == 0xFF40){
            std::cout<< "here3" << std::endl;
        }
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