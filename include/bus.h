#pragma once

#include <cstdint>
#include "cartridge.h"
#include "timer.h"
#include "ppu.h"

class CPU;

class Bus{
public:
    Bus(Cartridge& cart);

    /**
     * Reads a 16 bit address passing the handling of the memory retrieval to whatever section the memory points to
     * 
     * @param address 16 bit address to read from
     * @return byte stored at that address
     */
    uint8_t read(uint16_t address);
    /**
     * Same as read except there is no guard for dma
     */
    uint8_t readDuringDMA(uint16_t address);
    /**
     * Writes a byte to a specified 16 bit address by passing the write handling to whatever section the memory
     * points to
     * 
     * @param address 16 bit address to write to
     * @param byte the byte to write to that address
     */
    void write(uint16_t address, uint8_t byte);
    /**
     * advances all peripherals by tStates
     * 
     * @param tStates number of t states = 4xM-cycles
     * @param cpu reference to cpu
     */
    void step(int tStates, CPU& cpu);
    PPU ppu;
private:
    Cartridge& cart;
    Timer timer;

    // 8 KiB work ram
    uint8_t wram[0x2000];
    // High ram (127 bytes)
    uint8_t hram[0x7F];
    // IO registers
    uint8_t ioRegs[0x80];
    // Interrupt enable
    uint8_t ieReg = 0;
    uint8_t interruptFlag = 0xE1;

    // joyp register state for 0xFF00
    uint8_t joyp = 0xCF;
};