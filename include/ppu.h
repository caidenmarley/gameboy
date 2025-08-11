#pragma once
#include <cstdint>
#include <vector>
#include <deque>

class Bus;
class CPU;

class PPU{
public:
    PPU(Bus& bus);

    /**
     * Advance the PPU by tStates
     * 
     * @param tStates
     * @param cpu reference to cpu
     */
    void step(int tStates, CPU& cpu);

    /**
     * Access for PPU regions 0x8000-0xFF4B
     * 
     * @param address 16 bit address to read from
     * @return byte at the address
     */
    uint8_t read(uint16_t address) const;

    /**
     * Write byte to PPU memory region
     * 
     * @param address 16 bit address to write to
     * @param byte value to write
     */
    void write(uint16_t address, uint8_t byte);

    /**
     * Once full frame is rady copy pixels into 160x144 buffer
     * Pixel values are 0-3 indices
     */
    const uint8_t* getFrameBuffer() const{
        return frameBuffer;
    }

    bool isFrameReady() const{
        return frameReady;
    }

    bool isOamDmaActive() const{
        return oamDmaActive;
    }

    void clearNewFrameFlag(){
        frameReady = false;
    }
private:
    Bus& bus;

    uint8_t vram[0x2000]; // 8KiB
    uint8_t oam[0xA0]; // 160 bytes (40 sprites each 4 bytes)

    uint8_t LCDC; // LCD control FF40
    uint8_t STAT = 0; // LCD status FF41
    uint8_t SCY = 0; // Background viewport Y position FF42
    uint8_t SCX = 0; // Background viewport X position FF43
    uint8_t LY = 0; // LCD Y cooridnate (read only) FF44
    uint8_t LYC = 0; // LY compare FF45
    uint8_t BGP = 0; // BG palette data FF47 
    uint8_t OBP0 = 0xFF; // OBJ palette 0 data FF48
    uint8_t OBP1 = 0xFF; // OBJ palette 1 data FF49
    uint8_t WY = 0; // Window Y postition FF4A
    uint8_t WX = 0; // Window X position FF4B

    static constexpr uint16_t LCDC_ADDRESS = 0xFF40;
    static constexpr uint16_t STAT_ADDRESS = 0xFF41;
    static constexpr uint16_t SCY_ADDRESS = 0xFF42;
    static constexpr uint16_t SCX_ADDRESS = 0xFF43; 
    static constexpr uint16_t LY_ADDRESS = 0xFF44;
    static constexpr uint16_t LYC_ADDRESS = 0xFF45;
    static constexpr uint16_t BGP_ADDRESS = 0xFF47; 
    static constexpr uint16_t OBP0_ADDRESS = 0xFF48; 
    static constexpr uint16_t OBP1_ADDRESS = 0xFF49; 
    static constexpr uint16_t WY_ADDRESS = 0xFF4A; 
    static constexpr uint16_t WX_ADDRESS = 0xFF4B; 

    inline bool vramAccessible() const{ 
        if(!(LCDC & 0x80)) return true; // if LCDC bit 7 is disable VRAM is always accessible
        uint8_t m = STAT & 0x03;
        return m != 3;
    }
    inline bool oamAccessible() const{
        if(!(LCDC & 0x80)) return true; // if LCDC bit 7 is disable OAM is always accessible
        if (oamDmaActive) return false;
        uint8_t m = STAT & 0x03;
        return m == 0 || m == 1;
    }

    uint8_t frameBuffer[160*144];
    bool frameReady = false;

    int dotCounter = 0;

    /**
     * Computes an objects penalty for mode 3 based on how many stalls occur when a new 
     * background tile must be fetched mid object
     */
    int computeObjPenalty();

    int lastMode3Penalty;

    void renderScanline();

    struct Sprite{
        uint8_t y, x, tileIndex, flags; 
    };

    struct SpritePixel{
        uint8_t colour; // 2 bit colour
        bool bgPriority; // true if background has priority over this pixel
        uint8_t palette; // 0 = use OBP0, 1 = use OBP1
    };

    std::vector<Sprite> scanlineSprites;

    std::deque<uint8_t> backgroundFIFO; // background/window pixels
    std::deque<SpritePixel> spriteFIFO; // sprite pixels

    // to clear when LCD turns off
    void disableLCD(){
        STAT = (STAT & ~ 0x03) | 0;
        LY = 0;
        dotCounter = 0;
        backgroundFIFO.clear();
        spriteFIFO.clear();
        frameReady = false;
    }

    void enableLCD(){
        STAT = (STAT & ~0x03) | 2; // mode 2 for new frame
    }

    // OAM DMA state
    bool oamDmaActive = false;
    int oamDmaCycles = 0;
    uint16_t dmaSource = 0;

    // called each cpu step to advance DMA 
    void stepDma(int cycles){
        if(!oamDmaActive) return;
        oamDmaCycles -= cycles;
        if(oamDmaCycles <= 0){
            oamDmaActive = false;
        }
    }

    inline uint8_t vramReadRaw(uint16_t address) const{
        size_t index = address - 0x8000;
        if(index >= sizeof(vram)) return 0xFF;
        else return vram[index];
    }

    inline uint8_t oamReadRaw(uint16_t address) const{
        size_t index = address - 0xFE00;
        if(index >= sizeof(oam)) return 0xFF;
        else return oam[index];
    }
};
