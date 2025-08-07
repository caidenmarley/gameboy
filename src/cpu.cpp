#include "cpu.h"
#include "instructions.h"

CPU::CPU(Bus& bus) : bus(bus), A(0x01), F(0xB0), B(0), C(0x13), D(0), E(0xD8), H(0x01), L(0x4D), SP(0xFFFE), PC(0x0100){
    bus.write(INTERRUPT_FLAG_ADDRESS, 0xE1);
    bus.write(INTERRUPT_ENABLE_ADDRESS, 0x00);
}

int CPU::step(){
    if(imeEnabledNextStep){
        IME = true;
        imeEnabledNextStep = false;
    }

    // STOP
    if(stopped){
        uint8_t interruptRequest = bus.read(INTERRUPT_ENABLE_ADDRESS) & bus.read(INTERRUPT_FLAG_ADDRESS);
        if(interruptRequest & (1 << JOYPAD)){
            // if joypad requests interupt terminate "stop"
            stopped = false;
        }
        return 1;
    }

    // HALT
    if(halted){
        uint8_t interruptRequest = bus.read(INTERRUPT_ENABLE_ADDRESS) & bus.read(INTERRUPT_FLAG_ADDRESS);
        if(interruptRequest){
            // if any sort of interupt then terminate "halt"
            halted = false;
        }
        return 1;
    }

    uint8_t ieaValue = bus.read(INTERRUPT_ENABLE_ADDRESS);
    uint8_t ifaValue = bus.read(INTERRUPT_FLAG_ADDRESS);
    uint8_t interruptRequest = ieaValue & ifaValue;
    if(IME && interruptRequest){
        // if IME is enable and there are any interupt requests
        int src = 0;
        while ((interruptRequest & (1 << src)) == 0) {
            // interupt priority goes from VBLANK->JOYPAD, so service them one at a time in this order
            ++src;
        }
        // clear request
        bus.write(INTERRUPT_FLAG_ADDRESS, ifaValue & ~(1 << src));
        IME = false;

        // push return address
        pushToStack16(*this, bus, PC);
        // jump to interupt sources which start at 0x40
        PC = INTERRUPT_SOURCE_ADDRESS + src*8; // each address is 8 bits
        return 5; // 5 M-cycles

    }

    uint8_t opcode = bus.read(PC++);
    return decodeAndExecute(*this, bus, opcode);
}

void CPU::requestInterrupt(Interrupt interruptSource){
    uint8_t flag = bus.read(INTERRUPT_FLAG_ADDRESS);
    flag |= (1 << static_cast<uint8_t>(interruptSource));
    bus.write(INTERRUPT_FLAG_ADDRESS, flag);
}