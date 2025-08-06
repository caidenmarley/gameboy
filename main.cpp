#include "cartridge.h"
#include "cpu.h"
#include "timer.h"
#include "bus.h"
#include <iostream>
#include <string>

int main(){
    Cartridge cart;
    Bus bus(cart);
    CPU cpu(bus);
    const std::string path = "roms/pokemon-red.gb";
    cart.loadRom(path);
    bool running = true;

    while(running){
        int mcycles = cpu.step();
        int tstates = mcycles*4;
        bus.step(tstates, cpu);
    }
}