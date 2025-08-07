#include "timer.h"
#include "cpu.h"

Timer::Timer() : DIV(0xAB), TIMA(0x00), TMA(0x00), TAC(0xF8){}

void Timer::step(int tStates, CPU& cpu){
    // div increments at 16384Hz, so 64 mcycles, 256 tstates
    divCounter += tStates;
    while(divCounter >= 256){
        DIV = uint8_t(DIV + 1); // automatically wraps round 255+1-> 0
        divCounter -= 256; // remove the 256 that it wrapped around
    }

    // if TAC enable (bit 2)
    if(TAC & 0x04){
        int freq = FREQUENCIES[TAC & 0x03];
        timaCounter += tStates;

        while(timaCounter >= freq){
            timaCounter -= freq;
            if(TIMA == 0xFF){
                // overflow
                TIMA = TMA;
                cpu.requestInterrupt(CPU::TIMER);
            }else{
                TIMA++;
            }
        }
    }
}

uint8_t Timer::read(uint16_t address) const{
    switch(address){
        case DIV_ADDRESS: return DIV; // FF04
        case TIMA_ADDRESS: return TIMA; // FF05
        case TMA_ADDRESS: return TMA; // FF06
        case TAC_ADDRESS: return TAC; // FF07
        default: return 0xFF;
    }
}

void Timer::write(uint16_t address, uint8_t byte){
    switch(address){
        case DIV_ADDRESS: DIV = 0; divCounter = 0; break; // FF04 any write resets it and its counter
        case TIMA_ADDRESS: TIMA = byte; break; // FF05
        case TMA_ADDRESS: TMA = byte; break; // FF06
        case TAC_ADDRESS: TAC = byte & 0x07; break; // FF07, only bits 2-0 are used
        default: break;
    }
}

