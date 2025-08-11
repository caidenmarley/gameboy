#include "cartridge.h"
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <iostream> 

Cartridge::Cartridge() : currentRomBank(1), currentRamBank(0), ramEnabled(false), bankingMode(0), isMbc1(false), isMbc2(false), isMbc3(false),
mbc3RomBank(1), mbc3RamBank(0), mbc3RamRtcEnable(false), mbc3RtcSel(0xFF){}

static const uint8_t expectedNintendoLogo[48] = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

static size_t mapRamSize(uint8_t code){
    switch(code){
        case 0x00: return 0;    // no RAM
        case 0x01: return 0;    // MBC2 internal RAM is handled specially 
        case 0x02: return 1;    // 8 KiB
        case 0x03: return 4;    // 32 KiB (4x8KiB)
        case 0x04: return 16;   // 128 KiB (16x8Kib)
        case 0x05: return 8;    // 64 KiB (8x8KiB)
        default: return 0;
    }
}

bool validateHeader(const CartHeader& header, const std::vector<uint8_t>& romData){
    if(std::memcmp(header.nintendoLogo,expectedNintendoLogo, sizeof(expectedNintendoLogo)) != 0){
        throw std::runtime_error("Nintendo logo doesnt match");
    }

    uint8_t sum = 0;
    for(int i = 0x0134; i <= 0x014C; i++){
        sum = sum - romData[i] - 1;
    }
    if(sum != header.headerChecksum){
        throw std::runtime_error("Header checksum incorrect");
    }

    size_t expectedSize = 0x8000ULL << header.romSize;  // 0x8000 = 32 KiB
    if (romData.size() != expectedSize) {
        throw std::runtime_error(
            "ROM size (" + std::to_string(romData.size()) +
            " bytes) does not match header.romSize field (expected " +
            std::to_string(expectedSize) + " bytes)"
        );
    }

    return true;
}

bool Cartridge::loadRom(const std::string& romPath){
    this->romPath = romPath;

    std::ifstream rom(romPath, std::ios::binary | std::ios::ate);

    if (!rom.is_open()) {
        throw std::runtime_error("Failed to open file: " + romPath);
    }

    std::streamsize size = rom.tellg(); // get file size (pointer is at end)
    rom.seekg(0, std::ios::beg); // reset to beginning

    this->romData.resize(size);

    if (!rom.read(reinterpret_cast<char*>(this->romData.data()), size)) {
        throw std::runtime_error("Failed to read file: " + romPath);
    }

    if (size < 0x150) {
        throw std::runtime_error("ROM too small to contain a valid header");
    }

    std::memcpy(&this->header, this->romData.data()+0x100, sizeof(this->header));

    if(!validateHeader(this->header, this->romData)){
        return false;
    }

    uint8_t type = header.cartType;
    this->isMbc1 = (type == 0x01 || type == 0x02 || type == 0x03);
    this->isMbc2 = (type == 0x05 || type == 0x06);
    this->isMbc3 = (type == 0x0F || type == 0x10 || type == 0x11 || type == 0x12 || type == 0x13);

    if(isMbc2){
        this->ramData.resize(0x200);
    }else{
        ramData.resize(mapRamSize(this->header.ramSize) * 0x2000); // each bank 8KiB = 0x2000 bytes
    }

    // reset variables incase they changed with the previous instance of rom in the class
    this->currentRomBank = 1; // 0 is fixed bank
    this->currentRamBank = 0;
    this->ramEnabled = false;
    this->bankingMode = 0;

    std::cout << "ROM loaded sucessfully" << std::endl;

    return true;
}

void Cartridge::unloadRom(){
    this->romData.clear();
    this->ramData.clear();
    this->romPath.clear();
    header = CartHeader{};

    this->currentRomBank = 1; // 0 is fixed bank
    this->currentRamBank = 0;
    this->ramEnabled = false;
    this->bankingMode = 0;
}

uint8_t Cartridge::readByte(uint16_t address){
    // Mbc1
    if(isMbc1){
        if(address < 0x4000){
            size_t index = size_t(effectiveFixedBank()) * 0x4000 + address;
            return (index < romData.size()) ? romData[index] : 0xFF;
        }else if(address < 0x8000){
            uint8_t sb = effectiveSwitchBank();
            size_t index = size_t(sb) * 0x4000 + (address - 0x4000);
            return (index < romData.size()) ? romData[index] : 0xFF;
        }else if(address >= 0xA000 && address < 0xC000 && ramEnabled){
            size_t index = size_t(currentRamBank) * 0x2000 + (address - 0xA000);
            return (index < ramData.size()) ? ramData[index] : 0xFF;
        }else{
            return 0xFF;
        }
    }

    // Mbc2
    if(isMbc2){
        if(address < 0x4000){
            return romData[address];
        }else if(address < 0x8000){
            size_t index = currentRomBank * 0x4000 + (address - 0x4000);
            if(index >= romData.size()){
                throw std::runtime_error("Mbc2 address is out of romData range");
            }else{
                return romData[index];
            }
        }
        else if(address >= 0xA000 && address < 0xA200 && ramEnabled){
            return ramData[address - 0xA000];
        }
        return 0xFF;
    }

    // Mbc3
    if (isMbc3) {
        if (address < 0x4000) {
            // fixed bank 0
            return romData[address];
        } else if (address < 0x8000) {
            // switchable bank 1-127
            uint8_t bank = mbc3RomBank & 0x7F;
            if (bank == 0) bank = 1;
            size_t index = size_t(bank) * 0x4000 + (address - 0x4000);
            return (index < romData.size()) ? romData[index] : 0xFF;
        } else if (address >= 0xA000 && address < 0xC000) {
            if (!mbc3RamRtcEnable) return 0xFF;
            // if 0-3, map to ram banks
            if (mbc3RtcSel <= 0x03) {
                size_t index = size_t(mbc3RamBank & 0x03) * 0x2000 + (address - 0xA000);
                return (index < ramData.size()) ? ramData[index] : 0xFF;
            } else {
                // 0x08-0x0C rtc registers 
                return 0xFF;
            }
        }
        return 0xFF;
    }

    // No banking type 0x00
    if(address < 0x8000){
        return romData.at(address);
    }else if(address >= 0xA000 && address < 0xC000){
        size_t index = address - 0xA000;
        return (index < ramData.size() && ramEnabled) ? ramData.at(index) : 0xFF;
    }else{
        return 0xFF;
    }

}

bool Cartridge::writeByte(uint16_t address, uint8_t byte){
    // Mbc1
    if(isMbc1){
        if(address < 0x2000){
            // ram enable register sending command to set or clear ramEnabled state
            ramEnabled = ((byte & 0x0F) == 0x0A);   // any value with A in the lower 4 bits enables the ram
            return true;
        }else if(address < 0x4000){
            // rom bank number, selects rom bank number for the 4000-7FFF region higher bits are ignored leaving 5 lowest bits
            uint8_t newBank = byte & 0x1F; // 0x1F=00011111
            if(newBank == 0){
                newBank = 1;    // if reg set to 0 it behaves as if it is set to 1
            }
            // preserve any bits in 7,6,5 (0xE0=11100000) then or brings in the lower 5 bits
            currentRomBank = (currentRomBank & 0xE0) | newBank;
            return true;
        }else if(address < 0x6000){
            // ram bank number 2 bit reg controlled by banking mode
            uint8_t twoBits = byte & 0x03;
            if(bankingMode == 0){
                // on larger carts which need a >5 bit bank number this used to give additional 2 bits
                currentRomBank = (currentRomBank & 0x1F) | (twoBits << 5);
            }else{
                currentRamBank = twoBits;
            }
            return true;
        }else if(address < 0x8000){
            bankingMode = byte & 0x01; // low bit determines banking mode
            return true;
        }else if(address >= 0xA000 && address < 0xC000 && ramEnabled){
            size_t index = this->currentRamBank * 0x2000 + (address - 0xA000);
            if(index >= ramData.size()){
                throw std::runtime_error("writing data out of RAM range");
            }
            ramData[index] = byte;
            return true;
        }
    
        return false;
    }

    // Mbc2
    if(isMbc2){
        if(address < 0x2000 && ((address & 0x0100) == 0)){
            // ram enable
            ramEnabled = ((byte & 0x0F) == 0x0A);
            return true;
        }else if(address >= 0x2000 && address < 0x4000 && (address & 0x0100)){
            // rom bank
            uint8_t bank = byte & 0x0F;
            this->currentRomBank = (bank == 0 ? 1 : bank);
            return true;
        }else if(address >= 0xA000 && address < 0xA200 && ramEnabled){
            // ram write
            ramData[address - 0xA000] = byte & 0x0F; // low nibble only
            return true;
        }
        return false;
    }

    // MBC3
    if (isMbc3) {
        if (address < 0x2000) {
            // ram/rtc enable
            mbc3RamRtcEnable = ((byte & 0x0F) == 0x0A);
            return true;
        } else if (address < 0x4000) {
            // rom bank 7 bits
            mbc3RomBank = (byte & 0x7F);
            if (mbc3RomBank == 0) mbc3RomBank = 1;
            return true;
        } else if (address < 0x6000) {
            // ram bank (0-3) or rtc register select (0x08-0x0C)
            if ((byte & 0x0F) <= 0x03) {
                mbc3RamBank = byte & 0x03;
                mbc3RtcSel  = mbc3RamBank;
            } else {
                mbc3RtcSel = byte & 0x0F; // 0x08-0x0C
            }
            return true;
        } else if (address < 0x8000) {
            // Latch clock data 
            return true;
        } else if (address >= 0xA000 && address < 0xC000) {
            if (!mbc3RamRtcEnable) return false;
            if (mbc3RtcSel <= 0x03) {
                size_t index = size_t(mbc3RamBank & 0x03) * 0x2000 + (address - 0xA000);
                if (index < ramData.size()) {
                    ramData[index] = byte;
                    return true;
                }
                return false;
            } else {
                // rtc write
                return true;
            }
        }
        return false;
    }

    // No banking
    if(address >= 0xA000 && address < 0xC000 && ramEnabled){
        ramData[address - 0xA000] = byte;
        return true;
    }else{
        return false;
    }

}