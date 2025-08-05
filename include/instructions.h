#pragma once

#include <cstdint>
#include "bus.h"
#include "cpu.h"

void addToA(uint8_t value, CPU& cpu){
    uint16_t result = cpu.A + value;
    cpu.setFlag(cpu.zF, (result & 0xFF)==0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, ((cpu.A & 0x0F) + (value & 0x0F)) > 0x0F);
    cpu.setFlag(cpu.cF, result > 0xFF);
    cpu.A = result & 0xFF;
}

void addToACarry(uint8_t value, CPU& cpu){
    uint8_t carryFlag = cpu.getFlag(cpu.cF) ? 1 : 0;
    uint16_t result = cpu.A + value + carryFlag;
    cpu.setFlag(cpu.zF, (result & 0xFF)==0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, ((cpu.A & 0x0F) + (value & 0x0F) + carryFlag) > 0x0F);
    cpu.setFlag(cpu.cF, result > 0xFF);
    cpu.A = result & 0xFF;
}

void subFromA(uint8_t value, CPU& cpu){
    uint16_t result = cpu.A - value;
    cpu.setFlag(cpu.zF, (result & 0xFF)==0);
    cpu.setFlag(cpu.nF, true);
    cpu.setFlag(cpu.hF, (cpu.A & 0x0F) < (value & 0x0F));
    cpu.setFlag(cpu.cF, cpu.A < value);
    cpu.A = result & 0xFF;
}

void subFromACarry(uint8_t value, CPU& cpu){
    uint8_t carryFlag = cpu.getFlag(cpu.cF) ? 1 : 0;
    uint16_t result = cpu.A - value - carryFlag;
    cpu.setFlag(cpu.zF, (result & 0xFF)==0);
    cpu.setFlag(cpu.nF, true);
    cpu.setFlag(cpu.hF, (cpu.A & 0x0F) < ((value & 0x0F) + carryFlag));
    cpu.setFlag(cpu.cF, cpu.A < (value + carryFlag));
    cpu.A = result & 0xFF;
}

void andWithA(uint8_t value, CPU& cpu){
    cpu.A &= value;
    cpu.setFlag(cpu.zF, cpu.A==0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, true);
    cpu.setFlag(cpu.cF, false);
}

void xorWithA(uint8_t value, CPU& cpu){
    cpu.A ^= value;
    cpu.setFlag(cpu.zF, cpu.A==0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, false);
}

void orWithA(uint8_t value, CPU& cpu){
    cpu.A |= value;
    cpu.setFlag(cpu.zF, cpu.A==0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, false);
}

void compareWithA(uint8_t value, CPU& cpu){
    uint16_t result = cpu.A - value;
    cpu.setFlag(cpu.zF, (result & 0xFF)==0);
    cpu.setFlag(cpu.nF, true);
    cpu.setFlag(cpu.hF, (cpu.A & 0x0F) < (value & 0x0F));
    cpu.setFlag(cpu.cF, cpu.A < value);
}

uint16_t popFromStack16(CPU& cpu, Bus& bus){
    uint8_t low = bus.read(cpu.SP++);
    uint8_t high = bus.read(cpu.SP++);
    return uint16_t(high << 8) | uint16_t(low);
}

void pushToStack16(CPU& cpu, Bus& bus, uint16_t value){
    cpu.SP--;
    bus.write(cpu.SP, value >> 8);  // high byte
    cpu.SP--;
    bus.write(cpu.SP, value & 0xFF); // low byte
}

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
        case 0x11: // LD DE, d16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                cpu.DE(result);
                return 3;
            }
            break;
        case 0x12: // LD (DE), A
            {
                bus.write(cpu.DE(), cpu.A);
                return 2;
            }
            break;
        case 0x13: // INC DE
            {
                cpu.DE(cpu.DE() + 1);
                return 2;
            }
            break;
        case 0x14: // INC D
            {
                uint8_t result = cpu.D + 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.D & 0x0F) + 1 > 0x0F);
                cpu.D = result;
                return 1;
            }
            break;
        case 0x15: // DEC D
            {
                uint8_t result = cpu.D - 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, true);
                cpu.setFlag(cpu.hF, (cpu.D & 0x0F) == 0x00);
                cpu.D = result;
                return 1;
            }
            break;
        case 0x16: // LD D, d8
            {
                cpu.D = bus.read(cpu.PC++);
                return 2;
            }
            break;
        case 0x17: // RLA
            {
                uint8_t topBit = cpu.A & 0x80;
                cpu.A <<= 1;
                cpu.A |= cpu.getFlag(cpu.cF) ? 1 : 0;
                cpu.setFlag(cpu.zF, false);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, false);
                cpu.setFlag(cpu.cF, (topBit & 0x80) != 0);
                return 1;
            }
            break;
        case 0x18: // JR s8
            {
                int8_t offset = int8_t(bus.read(cpu.PC++));
                cpu.PC += offset;
                return 3;
            }
            break;
        case 0x19: // ADD HL, DE
            {
                uint32_t result = uint32_t(cpu.HL()) + uint32_t(cpu.DE());
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.HL() & 0x0FFF) + (cpu.DE() & 0x0FFF) > 0x0FFF);
                cpu.setFlag(cpu.cF, result > 0xFFFF);

                cpu.HL(result & 0xFFFF);
                return 2;
            }
            break;
        case 0x1A: // LD A, (DE)
            {
                cpu.A = bus.read(cpu.DE());
                return 2;
            }
            break;
        case 0x1B: // DEC DE
            {
                cpu.DE(cpu.DE() - 1);
                return 2;
            }
            break;
        case 0x1C: // INC E
            {
                uint8_t result = cpu.E + 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.E & 0x0F) + 1 > 0x0F);
                cpu.E = result;
                return 1;
            }
            break;
        case 0x1D: // DEC E
            {
                uint8_t result = cpu.E - 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, true);
                cpu.setFlag(cpu.hF, (cpu.E & 0x0F) == 0x00);
                cpu.E = result;
                return 1;
            }
            break;
        case 0x1E: // LD E, d8
            {
                cpu.E = bus.read(cpu.PC++);
                return 2;
            }
            break;
        case 0x1F: // RRA
            {
                uint8_t lowBit = cpu.A & 0x01;
                cpu.A = (cpu.A >> 1) | (cpu.getFlag(cpu.cF) ? 0x80 : 0);
                cpu.setFlag(cpu.zF, false);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, false);
                cpu.setFlag(cpu.cF, lowBit != 0);
                return 1;
            }
            break;
        case 0x20: // JR NZ, s8
            {
                int8_t offset = int8_t(bus.read(cpu.PC++));
                if(!cpu.getFlag(cpu.zF)){
                    // flag = 0
                    cpu.PC += offset;
                    return 3;
                }else{
                    return 2;
                }
            }
            break;
        case 0x21: // LD HL, d16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                cpu.HL(result);
                return 3;
            }
            break;
        case 0x22: // LD (HL+), A
            {
                bus.write(cpu.HL(), cpu.A);
                cpu.HL(cpu.HL() + 1);
                return 2;
            }
            break;
        case 0x23: // INC HL
            {
                cpu.HL(cpu.HL() + 1);
                return 2;
            }
            break;
        case 0x24: // INC H
            {
                uint8_t result = cpu.H + 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.H & 0x0F) + 1 > 0x0F);
                cpu.H = result;
                return 1;
            }
            break;
        case 0x25: // DEC H
            {
                uint8_t result = cpu.H - 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, true);
                cpu.setFlag(cpu.hF, (cpu.H & 0x0F) == 0x00);
                cpu.H = result;
                return 1;
            }
            break;
        case 0x26: // LD H, d8
            {
                cpu.H = bus.read(cpu.PC++);
                return 2;
            }
            break;
        case 0x27: // DAA
            {
                // BCD - each nibble represents a decimal digit (0-9) range of nibble values = 0000-1001 in binary
                // for lower nibble, you add or subract 0x60
                uint8_t correction = 0;
                bool setC = false;

                if(!cpu.getFlag(cpu.nF)){
                    // if not subtraction
                    if(cpu.getFlag(cpu.hF) || (cpu.A & 0x0F) > 9){
                        // if half carry or lower nibble is invalid
                        correction |= 0x06; // add 6, 0000 0110
                    }
                    if(cpu.getFlag(cpu.cF) || cpu.A > 0x99){
                        // if carry or A is too large to be a BCD value
                        correction |= 0x60; // 0110 0000
                        setC = true;
                    }
                    cpu.A += correction;
                }else{
                    // subtraction
                    if(cpu.getFlag(cpu.hF)){
                        correction |= 0x06;
                    }
                    if(cpu.getFlag(cpu.cF)){
                        correction |= 0x60;
                        setC = true;
                    }
                    cpu.A -= correction;
                }

                cpu.setFlag(cpu.zF, cpu.A == 0);
                cpu.setFlag(cpu.hF, false);
                cpu.setFlag(cpu.cF, setC);

                return 1;
            }
            break;
        case 0x28: // JR Z, s8
            {
                int8_t offset = int8_t(bus.read(cpu.PC++));
                if(cpu.getFlag(cpu.zF)){
                    cpu.PC += offset;
                    return 3;
                }else{
                    return 2;
                }
            }
            break;
        case 0x29: // ADD HL, HL
            {
                uint32_t result = uint32_t(cpu.HL()) + uint32_t(cpu.HL());
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.HL() & 0x0FFF) + (cpu.HL() & 0x0FFF) > 0x0FFF);
                cpu.setFlag(cpu.cF, result > 0xFFFF);

                cpu.HL(result & 0xFFFF);
                return 2;
            }
            break;
        case 0x2A: // LD A, (HL+)
            {
                cpu.A = bus.read(cpu.HL());
                cpu.HL(cpu.HL() + 1);
                return 2;
            }
            break;
        case 0x2B: // DEC HL
            {
                cpu.HL(cpu.HL() - 1);
                return 2;
            }
            break;
        case 0x2C: // INC L
            {
                uint8_t result = cpu.L + 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.L & 0x0F) + 1 > 0x0F);
                cpu.L = result;
                return 1;
            }
            break;
        case 0x2D: // DEC L
            {
                uint8_t result = cpu.L - 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, true);
                cpu.setFlag(cpu.hF, (cpu.L & 0x0F) == 0x00);
                cpu.L = result;
                return 1;
            }
            break;
        case 0x2E: // LD L, d8
            {
                cpu.L = bus.read(cpu.PC++);
                return 2;
            }
            break;
        case 0x2F: // CPL
            {
                cpu.A ^= 0xFF; 
                cpu.setFlag(cpu.nF, true);
                cpu.setFlag(cpu.hF, true);
            }
            break;
        case 0x30: // JR NC, s8
            {
                int8_t offset = int8_t(bus.read(cpu.PC++));
                if(!cpu.getFlag(cpu.cF)){
                    cpu.PC += offset;
                    return 3;
                }else{
                    return 2;
                }
            }
            break;
        case 0x31: // LD SP, d16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                cpu.SP = result;
                return 3;
            }
            break;
        case 0x32: // LD (HL-), A
            {
                bus.write(cpu.HL(), cpu.A);
                cpu.HL(cpu.HL() - 1);
                return 2;
            }
            break;
        case 0x33: // INC SP
            {
                cpu.SP++;
                return 2;
            }
            break;
        case 0x34: // INC (HL)
            {
                uint8_t value = bus.read(cpu.HL());
                uint8_t result = value + 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (value & 0x0F) + 1 > 0x0F);
                bus.write(cpu.HL(), result);
                return 3;
            }
            break;
        case 0x35: // DEC (HL)
            {
                uint8_t value = bus.read(cpu.HL());
                uint8_t result = value - 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, true);
                cpu.setFlag(cpu.hF, (value & 0x0F) == 0x00);
                bus.write(cpu.HL(), result);
                return 3;
            }
            break;
        case 0x36: // LD (HL), d8
            {
                bus.write(cpu.HL(), bus.read(cpu.PC++));
                return 3;
            }
            break;
        case 0x37: // SCF
            {
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, false);
                cpu.setFlag(cpu.cF, true);
                return 1;
            }
            break;
        case 0x38: // JR C, s8
            {
                int8_t offset = int8_t(bus.read(cpu.PC++));
                if(cpu.getFlag(cpu.cF)){
                    cpu.PC += offset;
                    return 3;
                }else{
                    return 2;
                }
            }
            break;
        case 0x39: // ADD HL, SP
            {
                uint32_t result = uint32_t(cpu.HL()) + uint32_t(cpu.SP);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.HL() & 0x0FFF) + (cpu.SP & 0x0FFF) > 0x0FFF);
                cpu.setFlag(cpu.cF, result > 0xFFFF);

                cpu.HL(result & 0xFFFF);
                return 2;
            }
            break;
        case 0x3A: // LD A, (HL-)
            {
                cpu.A = bus.read(cpu.HL());
                cpu.HL(cpu.HL() - 1);
                return 2;
            }
            break;
        case 0x3B: // DEC SP
            {
                cpu.SP--;
                return 2;
            }
            break;
        case 0x3C: // INC A
            {
                uint8_t result = cpu.A + 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, (cpu.A & 0x0F) + 1 > 0x0F);
                cpu.A = result;
                return 1;
            }
            break;
        case 0x3D: // DEC A
            {
                uint8_t result = cpu.A - 1;
                cpu.setFlag(cpu.zF, result==0);
                cpu.setFlag(cpu.nF, true);
                cpu.setFlag(cpu.hF, (cpu.A & 0x0F) == 0x00);
                cpu.A = result;
                return 1;
            }
            break;
        case 0x3E: // LD A, d8
            {
                cpu.A = bus.read(cpu.PC++);
                return 2;
            }
            break;
        case 0x3F: // CCF
            {
                if(cpu.getFlag(cpu.cF)){
                    cpu.setFlag(cpu.cF, false);
                }else{
                    cpu.setFlag(cpu.cF, true);
                }
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, false);
                return 1;
            }
            break;
        case 0x40: // LD B, B
            {
                cpu.B = cpu.B;
                return 1;
            }
            break;
        case 0x41: // LD B, C
            {
                cpu.B = cpu.C;
                return 1;
            }
            break;
        case 0x42: // LD B, D
            {
                cpu.B = cpu.D;
                return 1;
            }
            break;
        case 0x43: // LD B, E
            {
                cpu.B = cpu.E;
                return 1;
            }
            break;
        case 0x44: // LD B, H
            {
                cpu.B = cpu.H;
                return 1;
            }
            break;
        case 0x45: // LD B, L
            {
                cpu.B = cpu.L;
                return 1;
            }
            break;
        case 0x46: // LD B, (HL)
            {
                cpu.B = bus.read(cpu.HL());
                return 2;
            }
            break;
        case 0x47: // LD B, A
            {
                cpu.B = cpu.A;
                return 1;
            }
            break;
        case 0x48: // LD C, B
            {
                cpu.C = cpu.B;
                return 1;
            }
            break;
        case 0x49: // LD C, C
            {
                cpu.C = cpu.C;
                return 1;
            }
            break;
        case 0x4A: // LD C, D
            {
                cpu.C = cpu.D;
                return 1;
            }
            break;
        case 0x4B: // LD C, E
            {
                cpu.C = cpu.E;
                return 1;
            }
            break;
        case 0x4C: // LD C, H
            {
                cpu.C = cpu.H;
                return 1;
            }
            break;
        case 0x4D: // LD C, L
            {
                cpu.C = cpu.L;
                return 1;
            }
            break;
        case 0x4E: // LD C, (HL)
            {
                cpu.C = bus.read(cpu.HL());
                return 2;
            }
            break;
        case 0x4F: // LD C, A
            {
                cpu.C = cpu.A;
                return 1;
            }
            break;
        case 0x50: // LD D, B
            {
                cpu.D = cpu.B;
                return 1;
            }
            break;
        case 0x51: // LD D, C
            {
                cpu.D = cpu.C;
                return 1;
            }
            break;
        case 0x52: // LD D, D
            {
                cpu.D = cpu.D;
                return 1;
            }
            break;
        case 0x53: // LD D, E
            {
                cpu.D = cpu.E;
                return 1;
            }
            break;
        case 0x54: // LD D, H
            {
                cpu.D = cpu.H;
                return 1;
            }
            break;
        case 0x55: // LD D, L
            {
                cpu.D = cpu.L;
                return 1;
            }
            break;
        case 0x56: // LD D, (HL)
            {
                cpu.D = bus.read(cpu.HL());
                return 2;
            }
            break;
        case 0x57: // LD D, A
            {
                cpu.D = cpu.A;
                return 1;
            }
            break;
        case 0x58: // LD E, B
            {
                cpu.E = cpu.B;
                return 1;
            }
            break;
        case 0x59: // LD E, C
            {
                cpu.E = cpu.C;
                return 1;
            }
            break;
        case 0x5A: // LD E, D
            {
                cpu.E = cpu.D;
                return 1;
            }
            break;
        case 0x5B: // LD E, E
            {
                cpu.E = cpu.E;
                return 1;
            }
            break;
        case 0x5C: // LD E, H
            {
                cpu.E = cpu.H;
                return 1;
            }
            break;
        case 0x5D: // LD E, L
            {
                cpu.E = cpu.L;
                return 1;
            }
            break;
        case 0x5E: // LD E, (HL)
            {
                cpu.E = bus.read(cpu.HL());
                return 2;
            }
            break;
        case 0x5F: // LD E, A
            {
                cpu.E = cpu.A;
                return 1;
            }
            break;
        case 0x60: // LD H, B
            {
                cpu.H = cpu.B;
                return 1;
            }
            break;
        case 0x61: // LD H, C
            {
                cpu.H = cpu.C;
                return 1;
            }
            break;
        case 0x62: // LD H, D
            {
                cpu.H = cpu.D;
                return 1;
            }
            break;
        case 0x63: // LD H, E
            {
                cpu.H = cpu.E;
                return 1;
            }
            break;
        case 0x64: // LD H, H
            {
                cpu.H = cpu.H;
                return 1;
            }
            break;
        case 0x65: // LD H, L
            {
                cpu.H = cpu.L;
                return 1;
            }
            break;
        case 0x66: // LD H, (HL)
            {
                cpu.H = bus.read(cpu.HL());
                return 2;
            }
            break;
        case 0x67: // LD H, A
            {
                cpu.H = cpu.A;
                return 1;
            }
            break;
        case 0x68: // LD L, B
            {
                cpu.L = cpu.B;
                return 1;
            }
            break;
        case 0x69: // LD L, C
            {
                cpu.L = cpu.C;
                return 1;
            }
            break;
        case 0x6A: // LD L, D
            {
                cpu.L = cpu.D;
                return 1;
            }
            break;
        case 0x6B: // LD L, E
            {
                cpu.L = cpu.E;
                return 1;
            }
            break;
        case 0x6C: // LD L, H
            {
                cpu.L = cpu.H;
                return 1;
            }
            break;
        case 0x6D: // LD L, L
            {
                cpu.L = cpu.L;
                return 1;
            }
            break;
        case 0x6E: // LD L, (HL)
            {
                cpu.L = bus.read(cpu.HL());
                return 2;
            }
            break;
        case 0x6F: // LD L, A
            {
                cpu.L = cpu.A;
                return 1;
            }
            break;
        case 0x70: // LD (HL), B
            {
                bus.write(cpu.HL(), cpu.B);
                return 2;
            }
            break;
        case 0x71: // LD (HL), C
            {
                bus.write(cpu.HL(), cpu.C);
                return 2;
            }
            break;
        case 0x72: // LD (HL), D
            {
                bus.write(cpu.HL(), cpu.D);
                return 2;
            }
            break;
        case 0x73: // LD (HL), E
            {
                bus.write(cpu.HL(), cpu.E);
                return 2;
            }
            break;
        case 0x74: // LD (HL), H
            {
                bus.write(cpu.HL(), cpu.H);
                return 2;
            }
            break;
        case 0x75: // LD (HL), L
            {
                bus.write(cpu.HL(), cpu.L);
                return 2;
            }
            break;
        case 0x76: // HALT
            {
                // TODO HALT
            }
            break;
        case 0x77: // LD (HL), A
            {
                bus.write(cpu.HL(), cpu.A);
                return 2;
            }
            break;
        case 0x78: // LD A, B
            {
                cpu.A = cpu.B;
                return 1;
            }
            break;
        case 0x79: // LD A, C
            {
                cpu.A = cpu.C;
                return 1;
            }
            break;
        case 0x7A: // LD A, D
            {
                cpu.A = cpu.D;
                return 1;
            }
            break;
        case 0x7B: // LD A, E
            {
                cpu.A = cpu.E;
                return 1;
            }
            break;
        case 0x7C: // LD A, H
            {
                cpu.A = cpu.H;
                return 1;
            }
            break;
        case 0x7D: // LD A, L
            {
                cpu.A = cpu.L;
                return 1;
            }
            break;
        case 0x7E: // LD A, (HL)
            {
                cpu.A = bus.read(cpu.HL());
                return 2;
            }
            break;
        case 0x7F: // LD A, A
            {
                cpu.A = cpu.A;
                return 1;
            }
            break;
        case 0x80: // ADD A, B
            {
                addToA(cpu.B, cpu);
                return 1;
            }
            break;
        case 0x81: // ADD A, C
            {
                addToA(cpu.C, cpu);
                return 1;
            }
            break;
        case 0x82: // ADD A, D
            {
                addToA(cpu.D, cpu);
                return 1;
            }
            break;
        case 0x83: // ADD A, E
            {
                addToA(cpu.E, cpu);
                return 1;
            }
            break;
        case 0x84: // ADD A, H
            {
                addToA(cpu.H, cpu);
                return 1;
            }
            break;
        case 0x85: // ADD A, L
            {
                addToA(cpu.L, cpu);
                return 1;
            }
            break;
        case 0x86: // ADD A, (HL)
            {
                addToA(bus.read(cpu.HL()), cpu);
                return 2;
            }
            break;
        case 0x87: // ADD A, A
            {
                addToA(cpu.A, cpu);
                return 1;
            }
            break;
        case 0x88: // ADC A, B
            {
                addToACarry(cpu.B, cpu);
                return 1;
            }
            break;
        case 0x89: // ADC A, C
            {
                addToACarry(cpu.C, cpu);
                return 1;
            }
            break;
        case 0x8A: // ADC A, D
            {
                addToACarry(cpu.D, cpu);
                return 1;
            }
            break;
        case 0x8B: // ADC A, E
            {
                addToACarry(cpu.E, cpu);
                return 1;
            }
            break;
        case 0x8C: // ADC A, H
            {
                addToACarry(cpu.H, cpu);
                return 1;
            }
            break;
        case 0x8D: // ADC A, L
            {
                addToACarry(cpu.L, cpu);
                return 1;
            }
            break;
        case 0x8E: // ADC A, (HL)
            {
                addToACarry(bus.read(cpu.HL()), cpu);
                return 2;
            }
            break;
        case 0x8F: // ADC A, A
            {
                addToACarry(cpu.A, cpu);
                return 1;
            }
            break;
        case 0x90: // SUB B
            {
                subFromA(cpu.B, cpu);
                return 1;
            }
            break;
        case 0x91: // SUB C
            {
                subFromA(cpu.C, cpu);
                return 1;
            }
            break;
        case 0x92: // SUB D
            {
                subFromA(cpu.D, cpu);
                return 1;
            }
            break;
        case 0x93: // SUB E
            {
                subFromA(cpu.E, cpu);
                return 1;
            }
            break;
        case 0x94: // SUB H
            {
                subFromA(cpu.H, cpu);
                return 1;
            }
            break;
        case 0x95: // SUB L
            {
                subFromA(cpu.L, cpu);
                return 1;
            }
            break;
        case 0x96: // SUB (HL)
            {
                subFromA(bus.read(cpu.HL()), cpu);
                return 2;
            }
            break;
        case 0x97: // SUB A
            {
                subFromA(cpu.A, cpu);
                return 1;
            }
            break;
        case 0x98: // SBC A, B
            {
                subFromACarry(cpu.B, cpu);
                return 1;
            }
            break;
        case 0x99: // SBC A, C
            {
                subFromACarry(cpu.C, cpu);
                return 1;
            }
            break;
        case 0x9A: // SBC A, D
            {
                subFromACarry(cpu.D, cpu);
                return 1;
            }
            break;
        case 0x9B: // SBC A, E
            {
                subFromACarry(cpu.E, cpu);
                return 1;
            }
            break;
        case 0x9C: // SBC A, H
            {
                subFromACarry(cpu.H, cpu);
                return 1;
            }
            break;
        case 0x9D: // SBC A, L
            {
                subFromACarry(cpu.L, cpu);
                return 1;
            }
            break;
        case 0x9E: // SBC A, (HL)
            {
                subFromACarry(bus.read(cpu.HL()), cpu);
                return 2;
            }
            break;
        case 0x9F: // SBC A, A
            {
                subFromACarry(cpu.A, cpu);
                return 1;
            }
            break;
        case 0xA0: // AND B
            {
                andWithA(cpu.B, cpu);
                return 1;
            }
            break;
        case 0xA1: // AND C
            {
                andWithA(cpu.C, cpu);
                return 1;
            }
            break;
        case 0xA2: // AND D
            {
                andWithA(cpu.D, cpu);
                return 1;
            }
            break;
        case 0xA3: // AND E
            {
                andWithA(cpu.E, cpu);
                return 1;
            }
            break;
        case 0xA4: // AND H
            {
                andWithA(cpu.H, cpu);
                return 1;
            }
            break;
        case 0xA5: // AND L
            {
                andWithA(cpu.L, cpu);
                return 1;
            }
            break;
        case 0xA6: // AND (HL)
            {
                andWithA(bus.read(cpu.HL()), cpu);
                return 2;
            }
            break;
        case 0xA7: // AND A
            {
                andWithA(cpu.A, cpu);
                return 1;
            }
            break;
        case 0xA8: // XOR B
            {
                xorWithA(cpu.B, cpu);
                return 1;
            }
            break;
        case 0xA9: // XOR C
            {
                xorWithA(cpu.C, cpu);
                return 1;
            }
            break;
        case 0xAA: // XOR D
            {
                xorWithA(cpu.D, cpu);
                return 1;
            }
            break;
        case 0xAB: // XOR E
            {
                xorWithA(cpu.E, cpu);
                return 1;
            }
            break;
        case 0xAC: // XOR H
            {
                xorWithA(cpu.H, cpu);
                return 1;
            }
            break;
        case 0xAD: // XOR L
            {
                xorWithA(cpu.L, cpu);
                return 1;
            }
            break;
        case 0xAE: // XOR (HL)
            {
                xorWithA(bus.read(cpu.HL()), cpu);
                return 2;
            }
            break;
        case 0xAF: // XOR A
            {
                xorWithA(cpu.A, cpu);
                return 1;
            }
            break;
        case 0xB0: // OR B
            {
                orWithA(cpu.B, cpu);
                return 1;
            }
            break;
        case 0xB1: // OR C
            {
                orWithA(cpu.C, cpu);
                return 1;
            }
            break;
        case 0xB2: // OR D
            {
                orWithA(cpu.D, cpu);
                return 1;
            }
            break;
        case 0xB3: // OR E
            {
                orWithA(cpu.E, cpu);
                return 1;
            }
            break;
        case 0xB4: // OR H
            {
                orWithA(cpu.H, cpu);
                return 1;
            }
            break;
        case 0xB5: // OR L
            {
                orWithA(cpu.L, cpu);
                return 1;
            }
            break;
        case 0xB6: // OR (HL)
            {
                orWithA(bus.read(cpu.HL()), cpu);
                return 2;
            }
            break;
        case 0xB7: // OR A
            {
                orWithA(cpu.A, cpu);
                return 1;
            }
            break;
        case 0xB8: // CP B
            {
                compareWithA(cpu.B, cpu);
                return 1;
            }
            break;
        case 0xB9: // CP C
            {
                compareWithA(cpu.C, cpu);
                return 1;
            }
            break;
        case 0xBA: // CP D
            {
                compareWithA(cpu.D, cpu);
                return 1;
            }
            break;
        case 0xBB: // CP E
            {
                compareWithA(cpu.E, cpu);
                return 1;
            }
            break;
        case 0xBC: // CP H
            {
                compareWithA(cpu.H, cpu);
                return 1;
            }
            break;
        case 0xBD: // CP L
            {
                compareWithA(cpu.L, cpu);
                return 1;
            }
            break;
        case 0xBE: // CP (HL)
            {
                compareWithA(bus.read(cpu.HL()), cpu);
                return 2;
            }
            break;
        case 0xBF: // CP A
            {
                compareWithA(cpu.A, cpu);
                return 1;
            }
            break;
        case 0xC0: // RET NZ
            {
                if(!cpu.getFlag(cpu.zF)){
                    cpu.PC = popFromStack16(cpu, bus);
                    return 5;
                }else{
                    return 2;
                }
            }
            break;
        case 0xC1: // POP BC
            {
                cpu.BC(popFromStack16(cpu, bus));
                return 3;
            }
            break;
        case 0xC2: // JP NZ, a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                if(!cpu.getFlag(cpu.zF)){
                    cpu.PC = result;
                    return 4;
                }else{
                    return 3;
                }
            }
            break;
        case 0xC3: // JP a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                cpu.PC = result;
                return 4;
            }
            break; 
        case 0xC4: // CALL NZ, a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);

                if(!cpu.getFlag(cpu.zF)){
                    pushToStack16(cpu, bus, cpu.PC);
                    cpu.PC = result;
                    return 6;
                }else{
                    return 3;
                }
            }
            break;
        case 0xC5: // PUSH BC
            {
                pushToStack16(cpu, bus, cpu.BC());
                return 4;
            }
            break;
        case 0xC6: // ADD A, d8
            {
                addToA(bus.read(cpu.PC++), cpu);
                return 2;
            }
            break;
        case 0xC7: // RST 0 
            {
                pushToStack16(cpu, bus, cpu.PC);
                cpu.PC = 0x00;
                return 4;
            }
            break;
        case 0xC8: // RET Z
            {
                if(cpu.getFlag(cpu.zF)){
                    cpu.PC = popFromStack16(cpu, bus);
                    return 5;
                }else{
                    return 2;
                }
            }
            break;
        case 0xC9: // RET
            {
                cpu.PC = popFromStack16(cpu, bus);
                return 4;
            }
            break;
        case 0xCA: // JP Z, a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                if(cpu.getFlag(cpu.zF)){
                    cpu.PC = result;
                    return 4;
                }else{
                    return 3;
                }
            }
            break;
        case 0xCB:
            {
                // TODO 16 bit opcodes
            }
            break;
        case 0xCC: // CALL Z, a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                if(cpu.getFlag(cpu.zF)){
                    pushToStack16(cpu, bus, cpu.PC);
                    cpu.PC = result;
                    return 6;
                }else{
                    return 3;
                }               
            }
            break;
        case 0xCD: // CALL a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                pushToStack16(cpu, bus, cpu.PC);
                cpu.PC = result;
                return 6;
            }
            break;
        case 0xCE: // ADC A, d8
            {
                addToACarry(bus.read(cpu.PC++), cpu);
                return 2;
            }
            break;
        case 0xCF: // RST 1
            {
                pushToStack16(cpu, bus, cpu.PC);
                cpu.PC = 0x08;
                return 4;
            }
            break;
        case 0xD0: // RET NC
            {
                if(!cpu.getFlag(cpu.cF)){
                    cpu.PC = popFromStack16(cpu, bus);
                    return 5;
                }else{
                    return 2;
                }
            }
            break;
        case 0xD1: // POP DE
            {
                cpu.DE(popFromStack16(cpu, bus));
                return 3;
            }
            break;
        case 0xD2: // JP NC, a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                if(!cpu.getFlag(cpu.cF)){
                    cpu.PC = result;
                    return 4;
                }else{
                    return 3;
                }
            }
            break;
        case 0xD4: // CALL NC, a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                if(!cpu.getFlag(cpu.cF)){
                    pushToStack16(cpu, bus, cpu.PC);
                    cpu.PC = result;
                    return 6;
                }else{
                    return 3;
                }
            }
            break;
        case 0xD5: // PUSH DE
            {
                pushToStack16(cpu, bus, cpu.DE());
                return 4;
            }
            break;
        case 0xD6: // SUB d8
            {
                subFromA(bus.read(cpu.PC++), cpu);
                return 2;
            }
            break;
        case 0xD7: // RST 2
            {
                pushToStack16(cpu, bus, cpu.PC);
                cpu.PC = 0x10;
                return 4;
            }
            break;
        case 0xD8: // RET C
            {
                if(cpu.getFlag(cpu.cF)){
                    cpu.PC = popFromStack16(cpu, bus);
                    return 5;
                }else{
                    return 2;
                }
            }
            break;
        case 0xD9: // RETI
            {
                cpu.PC = popFromStack16(cpu, bus);
                cpu.IME = true;
                return 4;
            }
            break;
        case 0xDA: // JP C, a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                if(cpu.getFlag(cpu.cF)){
                    cpu.PC = result;
                    return 4;
                }else{
                    return 3;
                }
            }
            break;
        case 0xDC: // CALL C, a16
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                if(cpu.getFlag(cpu.cF)){
                    pushToStack16(cpu, bus, cpu.PC);
                    cpu.PC = result;
                    return 6;
                }else{
                    return 3;
                }
            }
            break;
        case 0xDE: // SBC A, d8
            {
                subFromACarry(bus.read(cpu.PC++), cpu);
                return 2;
            }
            break;
        case 0xDF: // RST 3
            {
                pushToStack16(cpu, bus, cpu.PC);
                cpu.PC = 0x18;
                return 4;
            }
            break;
        case 0xE0: // LD (a8), A
            {
                uint8_t offset = bus.read(cpu.PC++);
                bus.write(0xFF00 + offset, cpu.A);
                return 3;
            }
            break;
        case 0xE1: // POP HL
            {
                cpu.HL(popFromStack16(cpu, bus));
                return 3;
            }
            break;
        case 0xE2: // LD (C), A
            {
                bus.write(0xFF00 + cpu.C, cpu.A);
                return 2;
            }
            break;
        case 0xE5: // PUSH HL
            {
                pushToStack16(cpu, bus, cpu.HL());
                return 4;
            }
            break;
        case 0xE6: // AND d8
            {
                andWithA(bus.read(cpu.PC++), cpu);
                return 2;
            }
            break;
        case 0xE7: // RST 4
            {
                pushToStack16(cpu, bus, cpu.PC);
                cpu.PC = 0x20;
                return 4;
            }
            break;
        case 0xE8: // ADD SP, s8
            {
                int8_t value = int8_t(bus.read(cpu.PC++));
                uint16_t result = cpu.SP + value;
                cpu.setFlag(cpu.zF, false);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, ((cpu.SP & 0x0F) + (value & 0x0F)) > 0x0F);
                cpu.setFlag(cpu.cF, result > 0xFF);
                cpu.SP = result;
                return 4;
            }
            break;
        case 0xE9: // JP HL
            {
                cpu.PC = cpu.HL();
                return 1;
            }
            break;
        case 0xEA: // LD (a16), A
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                bus.write(result, cpu.A);
                return 4;
            }
            break;
        case 0xEE: // XOR d8
            {
                xorWithA(bus.read(cpu.PC++), cpu);
                return 2;
            }
            break;
        case 0xEF: // RST 5
            {
                pushToStack16(cpu, bus, cpu.PC);
                cpu.PC = 0x28;
                return 4;
            }
            break;
        case 0xF0: // LD A, (a8)
            {
                cpu.A = bus.read(0xFF00 + bus.read(cpu.PC++));
                return 3;
            }
            break;
        case 0xF1: // POP AF
            {
                cpu.AF(popFromStack16(cpu, bus));
                return 3;
            }
            break;
        case 0xF2: // LD A, (C)
            {
                cpu.A = bus.read(0xFF00 + cpu.C);
                return 2;
            }
            break;
        case 0xF3: // DI
            {
                cpu.IME = false;
                return 1;
            }
            break;
        case 0xF5: // PUSH AF
            {
                pushToStack16(cpu, bus, cpu.AF());
                return 4;
            }
            break;
        case 0xF6: // OR d8
            {
                orWithA(bus.read(cpu.PC++), cpu);
                return 2;
            }
            break;
        case 0xF7: // RST 6
            {
                pushToStack16(cpu, bus, cpu.PC);
                cpu.PC = 0x30;
                return 4;
            }
            break;
        case 0xF8: // LD HL, SP+s8
            {
                int8_t value = int8_t(bus.read(cpu.PC++));
                uint16_t result = cpu.SP + value;
                cpu.setFlag(cpu.zF, false);
                cpu.setFlag(cpu.nF, false);
                cpu.setFlag(cpu.hF, ((cpu.SP & 0x0F) + (value & 0x0F)) > 0x0F);
                cpu.setFlag(cpu.cF, result > 0xFF);
                cpu.HL(result);
                return 3;
            }
            break; 
        case 0xF9: // LD SP, HL
            {
                cpu.SP = cpu.HL();
                return 2;
            }
            break;
        case 0xFA: // LD A, (a16)
            {
                uint8_t low = bus.read(cpu.PC++);
                uint8_t high = bus.read(cpu.PC++);
                uint16_t result = uint16_t(high << 8) | uint16_t(low);
                cpu.A = bus.read(result);
                return 4;
            }
            break;
        case 0xFB: // EI
            {
                cpu.IME = true;
                return 1;
            }
            break;
        case 0xFE: // CP d8
            {
                compareWithA(bus.read(cpu.PC++), cpu);
                return 2;
            }
            break;
        case 0xFF: // RST 7
            {
                pushToStack16(cpu, bus, cpu.PC);
                cpu.PC = 0x38;
                return 4;
            }
            break;
    }
}