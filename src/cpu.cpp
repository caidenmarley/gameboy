#include "cpu.h"
#include "instructions.h"

CPU::CPU(Bus& bus) : bus(bus), A(0x01), F(0xB0), B(0), C(0x13), D(0), E(0xD8), H(0x01), L(0x4D), SP(0xFFFE), PC(0x0100){}

int CPU::step(){
    uint8_t opcode = bus.read(PC++);
    return decodeAndExecute(*this, bus, opcode);
}

void CPU::requestInterrupt(Interrupt interruptSource){
    uint8_t flag = bus.read(INTERRUPT_FLAG_ADDRESS);
}