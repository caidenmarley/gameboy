#pragma once

#include <cstdint>
#include "bus.h"
#include "cpu.h"

int decodeAndExecute(CPU& cpu, Bus& bus, uint8_t opcode){
    switch(opcode){
        case 0x00:  // NOP
            return 1;
            break;
        case 0x01: // LD BC, d16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                cpu.BC((high << 8) | low);
                return 3;
            }
            break;
        case 0x02: // LD (BC), A
            {
                bus.write(cpu.BC(), cpu.A);
                return 2;
            }
            break;
        case 0x03: // INC BC
            {
                cpu.BC(cpu.BC() + 1); 
                return 2;
            }
            break;
        case 0x04: // INC B
            {
                uint8_t result = cpu.B + 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.B & 0x0F) + 1 > 0x0F);
                cpu.B = result;
                return 1;
            }
            break;
        case 0x05: // DEC B
            {
                uint8_t result = cpu.B - 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, true);
                cpu.setFlag(cpu.hF, (cpu.B & 0x0F) == 0x00);
                cpu.B = result;
                return 1;
            }
            break;
        case 0x06: // LD B, d8
            {
                uint8_t value = bus.read(cpu.PC++);
                cpu.B = value;
                return 2;
            }
            break;
        case 0x07: // RLCA
            {
                uint8_t topBit = cpu.A & 0x80;
                cpu.A <<= 1;
                cpu.A |= (topBit >> 7);
                cpu.setFlag(cpu.zF, false);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, false);
                cpu.setFlag(cpu.cF, (topBit & 0x80) != 0);
                return 1;
            }
            break;
        case 0x08: // LD (a16), SP
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t address = (high << 8) | low;

                bus.write(address, cpu.SP & 0xFF); // low byte
                bus.write(address + 1, cpu.SP >> 8);

                return 5;
            }
            break;
        case 0x09: // ADD HL, BC
            {
                uint32_t result = uint32_t(cpu.BC()) + uint32_t(cpu.HL());
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.BC() & 0x0FFF) + (cpu.HL() & 0x0FFF) > 0x0FFF);
                cpu.setFlag(cpu.cF, result > 0xFFFF);

                cpu.HL(result & 0xFFFF);
                return 2;
            }
            break;
        case 0x0A: // LD A, (BC)
            {
                cpu.A = bus.read(cpu.BC());
                return 2;
            }
            break;
        case 0x0B: // DEC BC
            {
                cpu.BC(cpu.BC() - 1);
                return 2;
            }
            break;
        case 0x0C: // INC C
            {
                uint8_t result = cpu.C + 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.C & 0x0F) + 1 > 0x0F);
                cpu.C = result;
                return 1;
            }
            break;
        case 0x0D: // DEC C
            {
                uint8_t result = cpu.C - 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, true);
                cpu.setFlag(cpu.hF, (cpu.C & 0x0F) == 0x00);
                cpu.C = result;
                return 1;
            }
            break;
        case 0x0E: // LD C, d8
            {
                cpu.C = bus.read(cpu.PC++);
                return 2;
            }
            break;
        case 0x0F: // RRCA
            {
                uint8_t lowBit = cpu.A & 0x01;
                cpu.A = (cpu.A >> 1) | (lowBit << 7);
                cpu.setFlag(cpu.zF, false);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, false);
                cpu.setFlag(cpu.cF, lowBit != 0);
                return 1;
            }
            break;
        case 0x10: // STOP
            {
                //TODO
                return 1;
            }
            break;
    }
}