#include "cartridge.h"
#include "cpu.h"
#include "timer.h"
#include "bus.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-rom.gb>\n";
        return 1;
    }

    const std::string path = argv[1];
    Cartridge cart;

    if(!cart.loadRom(path)){
        std::cerr << "Failed to load ROM" << std::endl;
        return 1;
    }

    Bus bus(cart);
    CPU cpu(bus);

    while(true){
        int mcycles = cpu.step();
        int tstates = mcycles * 4;
        bus.step(tstates, cpu);

        if(bus.ppu.isFrameReady()){
            std:: cout << "." << std::flush;
        }

        bus.ppu.clearNewFrameFlag();
    }
}