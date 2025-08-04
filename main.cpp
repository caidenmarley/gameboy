#include "cartridge.h"
#include <iostream>
#include <string>

int main(){
    Cartridge cart;
    const std::string path = "roms/pokemon-red.gb";
    cart.loadRom(path);
}