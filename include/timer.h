#pragma once
#include <cstdint>

class CPU;

class Timer{
public:
    Timer();

    /**
     * Advance timer by number of t states
     * 
     * @param tStates one tState = one CPU clock cycle, One machine cycle (M-cycle) is 4 T-states
     * @param cpu reference to cpu used to request interupts
     */
    void step(int tStates, CPU& cpu);

    /**
     * Reads the timer addresses FF04-FF07
     * 
     * @param address the 16 bit address to read from
     * @return the byte stored at the corresponding timer register
     */
    uint8_t read(uint16_t address) const;
    /**
     * Write to address FF04-FF07
     * 
     * @param address the 16 bit address to write to
     * @param byte the byte you are writing to the address
     */
    void write(uint16_t address, uint8_t byte);
private:
    uint8_t DIV = 0; // Divider register FF04
    uint8_t TIMA = 0; // Timer counter FF05
    uint8_t TMA = 0; // Timer modulo FF06
    uint8_t TAC = 0; // Timer control FF07

    int divCounter = 0; // accumulates t states for DIV
    int timaCounter = 0; // accumulates t states for TIMA when enabled

    static constexpr int FREQUENCIES[4] = {
        1024, // 4096Hz 256 M-cycles
        16, // 262144Hz 4 M-cycles
        64, // 65536Hz 16 M-cycles
        256 // 16384Hz 64 M-cycles
    };

    static constexpr uint16_t DIV_ADDRESS = 0xFF04;
    static constexpr uint16_t TIMA_ADDRESS = 0xFF05;
    static constexpr uint16_t TMA_ADDRESS = 0xFF06;
    static constexpr uint16_t TAC_ADDRESS = 0xFF07;
};