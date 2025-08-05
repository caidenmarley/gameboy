#pragma once
#include "cpu.h"
#include "bus.h"

uint8_t rlc(uint8_t value, CPU& cpu){
    uint8_t wrapBit = (value  >> 7);
    uint8_t result = (value << 1) | wrapBit;
    cpu.setFlag(cpu.zF, result == 0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, wrapBit != 0);
    return result;
}

uint8_t rrc(uint8_t value, CPU& cpu){
    uint8_t wrapBit = value & 0x01;
    uint8_t result = (value >> 1) | (wrapBit << 7);
    cpu.setFlag(cpu.zF, result == 0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, wrapBit != 0);
    return result;
}

uint8_t rl(uint8_t value, CPU& cpu){
    uint8_t carryFlag   = cpu.getFlag(cpu.cF) ? 1 : 0;
    uint8_t wrapBit= (value >> 7);
    uint8_t result = (value << 1) | carryFlag;

    cpu.setFlag(cpu.zF, result == 0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, wrapBit != 0);
    return result;
}

uint8_t rr(uint8_t value, CPU& cpu){
    uint8_t carryFlag   = cpu.getFlag(cpu.cF) ? 1 : 0;
    uint8_t wrapBit= (value & 0x01);
    uint8_t result = (value >> 1) | (carryFlag << 7);

    cpu.setFlag(cpu.zF, result == 0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, wrapBit != 0);
    return result;
}

uint8_t sla(uint8_t value, CPU& cpu) {
    uint8_t old7   = (value >> 7);
    uint8_t result = value << 1;
    cpu.setFlag(cpu.zF, result == 0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, old7 != 0);
    return result;
}

uint8_t sra(uint8_t value, CPU& cpu) {
    uint8_t old0   = value & 0x01;
    uint8_t msb    = value & 0x80;
    uint8_t result = (value >> 1) | msb;
    cpu.setFlag(cpu.zF, result == 0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, old0 != 0);
    return result;
}

uint8_t swapNibbles(uint8_t value, CPU& cpu) {
    uint8_t result = (value << 4) | (value >> 4);
    cpu.setFlag(cpu.zF, result == 0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, false);
    return result;
}

uint8_t srl(uint8_t value, CPU& cpu) {
    uint8_t old0   = value & 0x01;
    uint8_t result = value >> 1;
    cpu.setFlag(cpu.zF, result == 0);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, false);
    cpu.setFlag(cpu.cF, old0 != 0);
    return result;
}

void bitTest(uint8_t value, int bit, CPU& cpu) {
    bool zero = ((value & (1 << bit)) == 0); // true when equal to 0, so flips bit
    cpu.setFlag(cpu.zF, zero);
    cpu.setFlag(cpu.nF, false);
    cpu.setFlag(cpu.hF, true);
}

inline uint8_t bitReset(uint8_t value, int bit) {
    return value & ~(1 << bit);
}

inline uint8_t bitSet(uint8_t value, int bit) {
    return value | (1 << bit);
}

int decodeAndExecute16(CPU& cpu, Bus& bus){
    uint8_t cbOpcode = bus.read(cpu.PC++);
    switch(cbOpcode){
        // RLC
        case 0x00: cpu.B = rlc(cpu.B, cpu); return 2;
        case 0x01: cpu.C = rlc(cpu.C, cpu); return 2;
        case 0x02: cpu.D = rlc(cpu.D, cpu); return 2;
        case 0x03: cpu.E = rlc(cpu.E, cpu); return 2;
        case 0x04: cpu.H = rlc(cpu.H, cpu); return 2;
        case 0x05: cpu.L = rlc(cpu.L, cpu); return 2;
        case 0x06: { 
            uint8_t result = rlc(bus.read(cpu.HL()), cpu); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x07: cpu.A = rlc(cpu.A, cpu); return 2;

        // RRC
        case 0x08: cpu.B = rrc(cpu.B, cpu); return 2;
        case 0x09: cpu.C = rrc(cpu.C, cpu); return 2;
        case 0x0A: cpu.D = rrc(cpu.D, cpu); return 2;
        case 0x0B: cpu.E = rrc(cpu.E, cpu); return 2;
        case 0x0C: cpu.H = rrc(cpu.H, cpu); return 2;
        case 0x0D: cpu.L = rrc(cpu.L, cpu); return 2;
        case 0x0E: { 
            uint8_t result = rrc(bus.read(cpu.HL()), cpu); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x0F: cpu.A = rrc(cpu.A, cpu); return 2;

        // RL
        case 0x10: cpu.B = rl(cpu.B, cpu); return 2;
        case 0x11: cpu.C = rl(cpu.C, cpu); return 2;
        case 0x12: cpu.D = rl(cpu.D, cpu); return 2;
        case 0x13: cpu.E = rl(cpu.E, cpu); return 2;
        case 0x14: cpu.H = rl(cpu.H, cpu); return 2;
        case 0x15: cpu.L = rl(cpu.L, cpu); return 2;
        case 0x16: { 
            uint8_t result = rl(bus.read(cpu.HL()), cpu); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x17: cpu.A = rl(cpu.A, cpu); return 2;

        // RR
        case 0x18: cpu.B = rr(cpu.B, cpu); return 2;
        case 0x19: cpu.C = rr(cpu.C, cpu); return 2;
        case 0x1A: cpu.D = rr(cpu.D, cpu); return 2;
        case 0x1B: cpu.E = rr(cpu.E, cpu); return 2;
        case 0x1C: cpu.H = rr(cpu.H, cpu); return 2;
        case 0x1D: cpu.L = rr(cpu.L, cpu); return 2;
        case 0x1E: { 
            uint8_t result = rr(bus.read(cpu.HL()), cpu); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x1F: cpu.A = rr(cpu.A, cpu); return 2;

        // SLA
        case 0x20: cpu.B = sla(cpu.B, cpu); return 2;
        case 0x21: cpu.C = sla(cpu.C, cpu); return 2;
        case 0x22: cpu.D = sla(cpu.D, cpu); return 2;
        case 0x23: cpu.E = sla(cpu.E, cpu); return 2;
        case 0x24: cpu.H = sla(cpu.H, cpu); return 2;
        case 0x25: cpu.L = sla(cpu.L, cpu); return 2;
        case 0x26: { 
            uint8_t result = sla(bus.read(cpu.HL()), cpu); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x27: cpu.A = sla(cpu.A, cpu); return 2;

        // SRA
        case 0x28: cpu.B = sra(cpu.B, cpu); return 2;
        case 0x29: cpu.C = sra(cpu.C, cpu); return 2;
        case 0x2A: cpu.D = sra(cpu.D, cpu); return 2;
        case 0x2B: cpu.E = sra(cpu.E, cpu); return 2;
        case 0x2C: cpu.H = sra(cpu.H, cpu); return 2;
        case 0x2D: cpu.L = sra(cpu.L, cpu); return 2;
        case 0x2E: { 
            uint8_t result = sra(bus.read(cpu.HL()), cpu); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x2F: cpu.A = sra(cpu.A, cpu); return 2;

        // SWAP
        case 0x30: cpu.B = swapNibbles(cpu.B, cpu); return 2;
        case 0x31: cpu.C = swapNibbles(cpu.C, cpu); return 2;
        case 0x32: cpu.D = swapNibbles(cpu.D, cpu); return 2;
        case 0x33: cpu.E = swapNibbles(cpu.E, cpu); return 2;
        case 0x34: cpu.H = swapNibbles(cpu.H, cpu); return 2;
        case 0x35: cpu.L = swapNibbles(cpu.L, cpu); return 2;
        case 0x36: { 
            uint8_t result = swapNibbles(bus.read(cpu.HL()), cpu); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x37: cpu.A = swapNibbles(cpu.A, cpu); return 2;

        // SRL
        case 0x38: cpu.B = srl(cpu.B, cpu); return 2;
        case 0x39: cpu.C = srl(cpu.C, cpu); return 2;
        case 0x3A: cpu.D = srl(cpu.D, cpu); return 2;
        case 0x3B: cpu.E = srl(cpu.E, cpu); return 2;
        case 0x3C: cpu.H = srl(cpu.H, cpu); return 2;
        case 0x3D: cpu.L = srl(cpu.L, cpu); return 2;
        case 0x3E: { 
            uint8_t result = srl(bus.read(cpu.HL()), cpu); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x3F: cpu.A = srl(cpu.A, cpu); return 2;

        // BIT bit,reg
        case 0x40: bitTest(cpu.B, 0, cpu); return 2;
        case 0x41: bitTest(cpu.C, 0, cpu); return 2;
        case 0x42: bitTest(cpu.D, 0, cpu); return 2;
        case 0x43: bitTest(cpu.E, 0, cpu); return 2;
        case 0x44: bitTest(cpu.H, 0, cpu); return 2;
        case 0x45: bitTest(cpu.L, 0, cpu); return 2;
        case 0x46:{ 
            bitTest(bus.read(cpu.HL()), 0, cpu); 
            return 3;
        } 
        case 0x47: bitTest(cpu.A, 0, cpu); return 2;
        case 0x48: bitTest(cpu.B, 1, cpu); return 2;
        case 0x49: bitTest(cpu.C, 1, cpu); return 2;
        case 0x4A: bitTest(cpu.D, 1, cpu); return 2;
        case 0x4B: bitTest(cpu.E, 1, cpu); return 2;
        case 0x4C: bitTest(cpu.H, 1, cpu); return 2;
        case 0x4D: bitTest(cpu.L, 1, cpu); return 2;
        case 0x4E:{ 
            bitTest(bus.read(cpu.HL()), 1, cpu); 
            return 3;
        }
        case 0x4F: bitTest(cpu.A, 1, cpu); return 2;
        case 0x50: bitTest(cpu.B, 2, cpu); return 2;
        case 0x51: bitTest(cpu.C, 2, cpu); return 2;
        case 0x52: bitTest(cpu.D, 2, cpu); return 2;
        case 0x53: bitTest(cpu.E, 2, cpu); return 2;
        case 0x54: bitTest(cpu.H, 2, cpu); return 2;
        case 0x55: bitTest(cpu.L, 2, cpu); return 2;
        case 0x56:{ 
            bitTest(bus.read(cpu.HL()), 2, cpu); 
            return 3;
        }
        case 0x57: bitTest(cpu.A, 2, cpu); return 2;
        case 0x58: bitTest(cpu.B, 3, cpu); return 2;
        case 0x59: bitTest(cpu.C, 3, cpu); return 2;
        case 0x5A: bitTest(cpu.D, 3, cpu); return 2;
        case 0x5B: bitTest(cpu.E, 3, cpu); return 2;
        case 0x5C: bitTest(cpu.H, 3, cpu); return 2;
        case 0x5D: bitTest(cpu.L, 3, cpu); return 2;
        case 0x5E:{ 
            bitTest(bus.read(cpu.HL()), 3, cpu); 
            return 3;
        }
        case 0x5F: bitTest(cpu.A, 3, cpu); return 2;
        case 0x60: bitTest(cpu.B, 4, cpu); return 2;
        case 0x61: bitTest(cpu.C, 4, cpu); return 2;
        case 0x62: bitTest(cpu.D, 4, cpu); return 2;
        case 0x63: bitTest(cpu.E, 4, cpu); return 2;
        case 0x64: bitTest(cpu.H, 4, cpu); return 2;
        case 0x65: bitTest(cpu.L, 4, cpu); return 2;
        case 0x66:{ 
            bitTest(bus.read(cpu.HL()), 4, cpu); 
            return 3;
        }
        case 0x67: bitTest(cpu.A, 4, cpu); return 2;
        case 0x68: bitTest(cpu.B, 5, cpu); return 2;
        case 0x69: bitTest(cpu.C, 5, cpu); return 2;
        case 0x6A: bitTest(cpu.D, 5, cpu); return 2;
        case 0x6B: bitTest(cpu.E, 5, cpu); return 2;
        case 0x6C: bitTest(cpu.H, 5, cpu); return 2;
        case 0x6D: bitTest(cpu.L, 5, cpu); return 2;
        case 0x6E:{ 
            bitTest(bus.read(cpu.HL()), 5, cpu); 
            return 3;
        }
        case 0x6F: bitTest(cpu.A, 5, cpu); return 2;
        case 0x70: bitTest(cpu.B, 6, cpu); return 2;
        case 0x71: bitTest(cpu.C, 6, cpu); return 2;
        case 0x72: bitTest(cpu.D, 6, cpu); return 2;
        case 0x73: bitTest(cpu.E, 6, cpu); return 2;
        case 0x74: bitTest(cpu.H, 6, cpu); return 2;
        case 0x75: bitTest(cpu.L, 6, cpu); return 2;
        case 0x76:{ 
            bitTest(bus.read(cpu.HL()), 6, cpu); 
            return 3;
        }
        case 0x77: bitTest(cpu.A, 6, cpu); return 2;
        case 0x78: bitTest(cpu.B, 7, cpu); return 2;
        case 0x79: bitTest(cpu.C, 7, cpu); return 2;
        case 0x7A: bitTest(cpu.D, 7, cpu); return 2;
        case 0x7B: bitTest(cpu.E, 7, cpu); return 2;
        case 0x7C: bitTest(cpu.H, 7, cpu); return 2;
        case 0x7D: bitTest(cpu.L, 7, cpu); return 2;
        case 0x7E:{ 
            bitTest(bus.read(cpu.HL()), 7, cpu); 
            return 3;
        }
        case 0x7F: bitTest(cpu.A, 7, cpu); return 2;

        // RES bit,reg 
        case 0x80: cpu.B = bitReset(cpu.B, 0); return 2;
        case 0x81: cpu.C = bitReset(cpu.C, 0); return 2;
        case 0x82: cpu.D = bitReset(cpu.D, 0); return 2;
        case 0x83: cpu.E = bitReset(cpu.E, 0); return 2;
        case 0x84: cpu.H = bitReset(cpu.H, 0); return 2;
        case 0x85: cpu.L = bitReset(cpu.L, 0); return 2;
        case 0x86:{ 
            uint8_t result = bitReset(bus.read(cpu.HL()), 0); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x87: cpu.A = bitReset(cpu.A, 0); return 2;
        case 0x88: cpu.B = bitReset(cpu.B, 1); return 2;
        case 0x89: cpu.C = bitReset(cpu.C, 1); return 2;
        case 0x8A: cpu.D = bitReset(cpu.D, 1); return 2;
        case 0x8B: cpu.E = bitReset(cpu.E, 1); return 2;
        case 0x8C: cpu.H = bitReset(cpu.H, 1); return 2;
        case 0x8D: cpu.L = bitReset(cpu.L, 1); return 2;
        case 0x8E:{ 
            uint8_t result = bitReset(bus.read(cpu.HL()), 1); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x8F: cpu.A = bitReset(cpu.A, 1); return 2;
        case 0x90: cpu.B = bitReset(cpu.B, 2); return 2;
        case 0x91: cpu.C = bitReset(cpu.C, 2); return 2;
        case 0x92: cpu.D = bitReset(cpu.D, 2); return 2;
        case 0x93: cpu.E = bitReset(cpu.E, 2); return 2;
        case 0x94: cpu.H = bitReset(cpu.H, 2); return 2;
        case 0x95: cpu.L = bitReset(cpu.L, 2); return 2;
        case 0x96:{ 
            uint8_t result = bitReset(bus.read(cpu.HL()), 2); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x97: cpu.A = bitReset(cpu.A, 2); return 2;
        case 0x98: cpu.B = bitReset(cpu.B, 3); return 2;
        case 0x99: cpu.C = bitReset(cpu.C, 3); return 2;
        case 0x9A: cpu.D = bitReset(cpu.D, 3); return 2;
        case 0x9B: cpu.E = bitReset(cpu.E, 3); return 2;
        case 0x9C: cpu.H = bitReset(cpu.H, 3); return 2;
        case 0x9D: cpu.L = bitReset(cpu.L, 3); return 2;
        case 0x9E:{ 
            uint8_t result = bitReset(bus.read(cpu.HL()), 3); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0x9F: cpu.A = bitReset(cpu.A, 3); return 2;
        case 0xA0: cpu.B = bitReset(cpu.B, 4); return 2;
        case 0xA1: cpu.C = bitReset(cpu.C, 4); return 2;
        case 0xA2: cpu.D = bitReset(cpu.D, 4); return 2;
        case 0xA3: cpu.E = bitReset(cpu.E, 4); return 2;
        case 0xA4: cpu.H = bitReset(cpu.H, 4); return 2;
        case 0xA5: cpu.L = bitReset(cpu.L, 4); return 2;
        case 0xA6:{ 
            uint8_t result = bitReset(bus.read(cpu.HL()), 4); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0xA7: cpu.A = bitReset(cpu.A, 4); return 2;
        case 0xA8: cpu.B = bitReset(cpu.B, 5); return 2;
        case 0xA9: cpu.C = bitReset(cpu.C, 5); return 2;
        case 0xAA: cpu.D = bitReset(cpu.D, 5); return 2;
        case 0xAB: cpu.E = bitReset(cpu.E, 5); return 2;
        case 0xAC: cpu.H = bitReset(cpu.H, 5); return 2;
        case 0xAD: cpu.L = bitReset(cpu.L, 5); return 2;
        case 0xAE:{ 
            uint8_t result = bitReset(bus.read(cpu.HL()), 5); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0xAF: cpu.A = bitReset(cpu.A, 5); return 2;
        case 0xB0: cpu.B = bitReset(cpu.B, 6); return 2;
        case 0xB1: cpu.C = bitReset(cpu.C, 6); return 2;
        case 0xB2: cpu.D = bitReset(cpu.D, 6); return 2;
        case 0xB3: cpu.E = bitReset(cpu.E, 6); return 2;
        case 0xB4: cpu.H = bitReset(cpu.H, 6); return 2;
        case 0xB5: cpu.L = bitReset(cpu.L, 6); return 2;
        case 0xB6:{ 
            uint8_t result = bitReset(bus.read(cpu.HL()), 6); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0xB7: cpu.A = bitReset(cpu.A, 6); return 2;
        case 0xB8: cpu.B = bitReset(cpu.B, 7); return 2;
        case 0xB9: cpu.C = bitReset(cpu.C, 7); return 2;
        case 0xBA: cpu.D = bitReset(cpu.D, 7); return 2;
        case 0xBB: cpu.E = bitReset(cpu.E, 7); return 2;
        case 0xBC: cpu.H = bitReset(cpu.H, 7); return 2;
        case 0xBD: cpu.L = bitReset(cpu.L, 7); return 2;
        case 0xBE:{ 
            uint8_t result = bitReset(bus.read(cpu.HL()), 7); 
            bus.write(cpu.HL(), result); 
            return 4; 
        }
        case 0xBF: cpu.A = bitReset(cpu.A, 7); return 2;

        // SET bit,reg
        case 0xC0: cpu.B = bitSet(cpu.B, 0); return 2;
        case 0xC1: cpu.C = bitSet(cpu.C, 0); return 2;
        case 0xC2: cpu.D = bitSet(cpu.D, 0); return 2;
        case 0xC3: cpu.E = bitSet(cpu.E, 0); return 2;
        case 0xC4: cpu.H = bitSet(cpu.H, 0); return 2;
        case 0xC5: cpu.L = bitSet(cpu.L, 0); return 2;
        case 0xC6:{ 
            bus.write(cpu.HL(), bitSet(bus.read(cpu.HL()), 0)); 
            return 4; 
        }
        case 0xC7: cpu.A = bitSet(cpu.A, 0); return 2;
        case 0xC8: cpu.B = bitSet(cpu.B, 1); return 2;
        case 0xC9: cpu.C = bitSet(cpu.C, 1); return 2;
        case 0xCA: cpu.D = bitSet(cpu.D, 1); return 2;
        case 0xCB: cpu.E = bitSet(cpu.E, 1); return 2;
        case 0xCC: cpu.H = bitSet(cpu.H, 1); return 2;
        case 0xCD: cpu.L = bitSet(cpu.L, 1); return 2;
        case 0xCE:{ 
            bus.write(cpu.HL(), bitSet(bus.read(cpu.HL()), 1)); 
            return 4; 
        }
        case 0xCF: cpu.A = bitSet(cpu.A, 1); return 2;
        case 0xD0: cpu.B = bitSet(cpu.B, 2); return 2;
        case 0xD1: cpu.C = bitSet(cpu.C, 2); return 2;
        case 0xD2: cpu.D = bitSet(cpu.D, 2); return 2;
        case 0xD3: cpu.E = bitSet(cpu.E, 2); return 2;
        case 0xD4: cpu.H = bitSet(cpu.H, 2); return 2;
        case 0xD5: cpu.L = bitSet(cpu.L, 2); return 2;
        case 0xD6:{ 
            bus.write(cpu.HL(), bitSet(bus.read(cpu.HL()), 2)); 
            return 4; 
        }
        case 0xD7: cpu.A = bitSet(cpu.A, 2); return 2;
        case 0xD8: cpu.B = bitSet(cpu.B, 3); return 2;
        case 0xD9: cpu.C = bitSet(cpu.C, 3); return 2;
        case 0xDA: cpu.D = bitSet(cpu.D, 3); return 2;
        case 0xDB: cpu.E = bitSet(cpu.E, 3); return 2;
        case 0xDC: cpu.H = bitSet(cpu.H, 3); return 2;
        case 0xDD: cpu.L = bitSet(cpu.L, 3); return 2;
        case 0xDE:{ 
            bus.write(cpu.HL(), bitSet(bus.read(cpu.HL()), 3)); 
            return 4; 
        }
        case 0xDF: cpu.A = bitSet(cpu.A, 3); return 2;
        case 0xE0: cpu.B = bitSet(cpu.B, 4); return 2;
        case 0xE1: cpu.C = bitSet(cpu.C, 4); return 2;
        case 0xE2: cpu.D = bitSet(cpu.D, 4); return 2;
        case 0xE3: cpu.E = bitSet(cpu.E, 4); return 2;
        case 0xE4: cpu.H = bitSet(cpu.H, 4); return 2;
        case 0xE5: cpu.L = bitSet(cpu.L, 4); return 2;
        case 0xE6:{ 
            bus.write(cpu.HL(), bitSet(bus.read(cpu.HL()), 4)); 
            return 4; 
        }
        case 0xE7: cpu.A = bitSet(cpu.A, 4); return 2;
        case 0xE8: cpu.B = bitSet(cpu.B, 5); return 2;
        case 0xE9: cpu.C = bitSet(cpu.C, 5); return 2;
        case 0xEA: cpu.D = bitSet(cpu.D, 5); return 2;
        case 0xEB: cpu.E = bitSet(cpu.E, 5); return 2;
        case 0xEC: cpu.H = bitSet(cpu.H, 5); return 2;
        case 0xED: cpu.L = bitSet(cpu.L, 5); return 2;
        case 0xEE:{ 
            bus.write(cpu.HL(), bitSet(bus.read(cpu.HL()), 5)); 
            return 4; 
        }
        case 0xEF: cpu.A = bitSet(cpu.A, 5); return 2;
        case 0xF0: cpu.B = bitSet(cpu.B, 6); return 2;
        case 0xF1: cpu.C = bitSet(cpu.C, 6); return 2;
        case 0xF2: cpu.D = bitSet(cpu.D, 6); return 2;
        case 0xF3: cpu.E = bitSet(cpu.E, 6); return 2;
        case 0xF4: cpu.H = bitSet(cpu.H, 6); return 2;
        case 0xF5: cpu.L = bitSet(cpu.L, 6); return 2;
        case 0xF6:{ 
            bus.write(cpu.HL(), bitSet(bus.read(cpu.HL()), 6)); 
            return 4; 
        }
        case 0xF7: cpu.A = bitSet(cpu.A, 6); return 2;
        case 0xF8: cpu.B = bitSet(cpu.B, 7); return 2;
        case 0xF9: cpu.C = bitSet(cpu.C, 7); return 2;
        case 0xFA: cpu.D = bitSet(cpu.D, 7); return 2;
        case 0xFB: cpu.E = bitSet(cpu.E, 7); return 2;
        case 0xFC: cpu.H = bitSet(cpu.H, 7); return 2;
        case 0xFD: cpu.L = bitSet(cpu.L, 7); return 2;
        case 0xFE:{ 
            bus.write(cpu.HL(), bitSet(bus.read(cpu.HL()), 7)); 
            return 4; 
        }
        case 0xFF: cpu.A = bitSet(cpu.A, 7); return 2;

        default:
            return 0;
    }
}
