#pragma once
#include "bus.h"

class CPU{
public:
    /**
     * Initialises registers to default start up value for "DMG"
     * 
     * @param bus reference to main bus
     */
    CPU(Bus& bus);

    /**
     * Steps through opcodes, fetching decoding and executing returning the amount
     * of machine cycles the step took
     * 
     * @return number of M cycles
     */
    int step();
    
    Bus& bus;

    // regs
    uint8_t A, F;
    uint8_t B, C;
    uint8_t D, E;
    uint8_t H, L;
    uint16_t SP;
    uint16_t PC;

    uint16_t AF() const{
        return (uint16_t(A) << 8) | F;
    }
    void AF(uint16_t val){
        A = val >> 8;
        F = val & 0xF0; // 0xF0 as only the upper 4 bits of F are valid flags
    }

    uint16_t BC() const{
        return (uint16_t(B) << 8) | C;
    }
    void BC(uint16_t val){
        B = val >> 8;
        C = val & 0xFF;
    }

    uint16_t DE() const{
        return (uint16_t(D) << 8) | E;
    }
    void DE(uint16_t val){
        D = val >> 8;
        E = val & 0xFF;
    }

    uint16_t HL() const{
        return (uint16_t(H) << 8) | L;
    }
    void HL(uint16_t val){
        H = val >> 8;
        L = val & 0xFF;
    }

    // flags
    static constexpr uint8_t zF = 0x80; // zero flag, 0x80 isolates bit 7
    static constexpr uint8_t nF = 0x40; // subtraction flag, 0x40 isolates bit 6
    static constexpr uint8_t hF = 0x20; // half carry flag, 0x20 isolates bit 5
    static constexpr uint8_t cF = 0x10; // carry flag, 0x10 isolates bit 4

    void setFlag(uint8_t isolatingBit, bool on){
        if(on){
            F |= isolatingBit;
        }else{
            F &= ~isolatingBit;
        }
    }

    bool getFlag(uint8_t isolatingBit) const{
        return (F & isolatingBit) != 0; // if flag bit is 0, expression == 0, so false, else true
    }

    bool halted = false;
    bool stopped = false;
    
    // Interupts
    enum Interrupt : uint8_t {
        VBLANK = 0,
        LCD = 1,
        TIMER = 2,
        SERIAL = 3, 
        JOYPAD = 4
    };
    
    static constexpr uint16_t INTERRUPT_FLAG_ADDRESS = 0xFF0F;
    static constexpr uint16_t INTERRUPT_ENABLE_ADDRESS = 0xFFFF;
    static constexpr uint16_t INTERRUPT_SOURCE_ADDRESS = 0x40;
    
    bool imeEnabledNextStep = false;
    bool IME = false; // Interrupt Master Enable

    /**
     * Called by things like PPU, timer or joypad when an interupt should be signalled
     * 
     * @param interruptSource the thing requesting the interupt
     */
    void requestInterrupt(Interrupt interruptSource);
};