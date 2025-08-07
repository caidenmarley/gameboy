#pragma once
#include <cstdint>
#include <string>
#include <vector>

// pragma wrap ensures no packing bytes inbetween data so memcpy can be used
#pragma pack(push,1)
struct CartHeader{
    uint8_t entry[4];   // 0100-0103
    uint8_t nintendoLogo[48];   // 0104-0133
    union{
        uint8_t rawTitle[16]; // 0134-0143
        struct{
            char title[11]; // 0x0134-0x013E
            char manufacturerCode[4]; // 013F-0142
            uint8_t CGBFlag; // 0143
        } info;
    };
    uint8_t newLicenseeCode[2]; // 0144-0145
    uint8_t SGBFlag; // 0146
    uint8_t cartType; // 0147
    uint8_t romSize; // 0148
    uint8_t ramSize; // 0149
    uint8_t destinationCode; // 014A
    uint8_t oldLicenseeCode; // 014B
    uint8_t maskRomVersionNumber; // 014C
    uint8_t headerChecksum; // 014D
    uint8_t globalChecksum[2]; // 014E-014F
};
#pragma pack(pop)

// ensure header isnt too big/small
static_assert(sizeof(CartHeader) == 0x50, "CartHeader must be exactly 80 bytes");

class Cartridge{
public:
    Cartridge();

    /**
     * Loads ROM at the filepath into memory
     * 
     * @param romPath the location of the rom
     * @return true if successfully loaded, false if not
     */
    bool loadRom(const std::string& romPath);
    /**
     * unloads ROM from memory so class instance can be used again
     */
    void unloadRom();

    /**
     * Reads a byte from the stored ROM
     * 
     * @param 16 bit address to read from
     * @return byte stored at the address
     */
    uint8_t readByte(uint16_t address);
    /**
     * Write a byte into the stored ROM
     * 
     * @param address 16 bit address to write to
     * @param byte the byte to be written
     * @return true if successfully written, false if not
     */
    bool writeByte(uint16_t address, uint8_t byte);
private:
    CartHeader header;
    std::string romPath;
    std::vector<uint8_t> romData;
    std::vector<uint8_t> ramData;

    uint8_t currentRomBank;
    uint8_t currentRamBank;
    bool ramEnabled;
    uint8_t bankingMode;   // 0 ROM banking, 1 RAM banking
    bool isMbc1;
    bool isMbc2;
};