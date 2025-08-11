#include "ppu.h"
#include "cpu.h"
#include <cstring>
#include <unordered_set> // just keys no values
#include <vector>
#include <algorithm> // stable_sort
#include <iostream>

PPU::PPU(Bus& bus) : bus(bus), LCDC(0x91), STAT(0x85), SCY(0x00), SCX(0x00),
                     LY(0x00), LYC(0x00), BGP(0xFC), WY(0x00), WX(0x00){
    scanlineSprites.reserve(10);
    std::memset(vram, 0, sizeof(vram));
    std::memset(oam, 0, sizeof(oam));
}

void PPU::step(int tStates, CPU& cpu){
    stepDma(tStates);

    if(!(LCDC & 0x80)) return; // LCDC.7 is enable if off ppu is idle

    dotCounter += tStates;

    while(true){
        uint8_t mode = STAT & 0x03;
        int needed = 0;

        switch(mode){
            case 2: needed = 80; break; // OAM scan = 80 dots
            case 3: { // pixel transfer, length can be different
                int base = 160; // outputs one pixel to the screen per dot, 160 pixels wide
                int basePenalty = 12; // 12 dot penalty for initial two tile fetches
                int scrollPenalty = SCX % 8; // background scroll penalty
                int windowPenalty = ((LCDC & 0x20) && LY >= WY) ? 6 : 0; // after last non window pxiel a 6 dot penalty is incurred
                int objPenalty = computeObjPenalty();
                needed = base + basePenalty + scrollPenalty + windowPenalty + objPenalty;
            }
            break;
            case 0: { // HBlank
                int mode3length = 172 + (SCX % 8) + (((LCDC & 0x20) && LY >= WY) ? 6 : 0) + lastMode3Penalty;
                needed = 376 - mode3length;
            }
            break;
            case 1: needed = 456; break; // VBlank, each line is 456 dots
            default: needed = 0; break;
        }

        if (dotCounter < needed) break;
        dotCounter -= needed; // use up the required dots

        switch(mode){
            case 2: {
                // Select up to 10 sprites whose LY+16 overlaps the sprites Y range (set of scalines it covers vertically)
                scanlineSprites.clear();
                int spriteHeight = (LCDC & (1 << 2)) ? 16 : 8; // LCDC bit 2 determines sprite size 8x8 or 8x16
                // objects vertical position on screen + 16 = byte 0 of 4 byte sprite chunk in oam
                int line = LY + 16;
                for(int i = 0; i < 40; i++){
                    uint8_t oY = oam[i*4 + 0];
                    uint8_t oX = oam[i*4 + 1];
                    if(line >= oY && line < oY + spriteHeight){
                        scanlineSprites.push_back(
                            {oY, oX, oam[i*4 + 2], oam[i*4 + 3]}
                        );
                        if(scanlineSprites.size() == 10){
                            break;
                        }
                    }
                }
                // OAM scan finished, enter mode 3
                STAT = (STAT & ~0x03) | 3;
            }
            break;
            case 3: {
                renderScanline();
                // switch to mode 0
                STAT = (STAT & ~0x03) | 0;
                if(STAT & (1 << 3)){
                    // if mode 0 interrupt is enabled bit 3, request it
                    cpu.requestInterrupt(CPU::Interrupt::LCD);
                }
            }
            break;
            case 0: {
                // advance to next scanline
                LY++;

                // when LY = LYC bit 2 of STAT is set
                if (LY == LYC){
                    STAT |= (1 << 2);
                    if(STAT & (1 << 6)){ // bit 6 STAT = LYC=LY interupt enable
                        cpu.requestInterrupt(CPU::Interrupt::LCD);
                    }
                }else{
                    STAT &= ~(1 << 2);
                }

                if(LY == 144){
                    // enter vblank mode line 144 to 153
                    STAT = (STAT & ~0x03) | 1; // mode 1
                    frameReady = true;

                    cpu.requestInterrupt(CPU::Interrupt::VBLANK);

                    // STAT mode 1 interupt if enabled
                    if(STAT & (1 << 4)){    // STAT bit 4 mode 1 int enable
                        cpu.requestInterrupt(CPU::Interrupt::LCD);
                    }
                }else if(LY > 153){
                    // wrap back to line 0 after VBLANK lines
                    LY = 0;
                    STAT = (STAT & ~0x03) | 2; // Mode 2 for new frames first scanline
                }else{
                    // more visible lines mode 2
                    STAT = (STAT & ~0x03) | 2;
                    if(STAT & (1 << 5)){    // STAT bit 5 mode 2 interupt enable
                        cpu.requestInterrupt(CPU::Interrupt::LCD);
                    }
                }
            } 
            break;
            case 1: {
                LY++;
                if(LY == LYC){
                    STAT |= (1 << 2);
                    if(STAT & (1 << 6)){
                        cpu.requestInterrupt(CPU::Interrupt::LCD);
                    }
                }else{
                    STAT &= ~(1 << 2);
                }

                if(LY > 153){
                    // end of vblank, new frame
                    LY = 0;
                    STAT = (STAT & ~0x03) | 2;
                    // if mode 2 stat interupts are enabled request one
                    if(STAT & (1 << 5)){ // STAT bit 5 mode 2 interupt enable
                        cpu.requestInterrupt(CPU::Interrupt::LCD);
                    }
                }else{
                    STAT = (STAT & ~0x03) | 1;
                }
            }
            break;
        }
    }
}

uint8_t PPU::read(uint16_t address) const{
    // VRAM 8000-9FFF, cant access during mode 3, pixel transfer
    if(address >= 0x8000 && address <= 0x9FFF){
        return vramAccessible() ? vram[address - 0x8000] : 0xFF;
    }

    // OAM FE00-FE9F
    if(address >= 0xFE00 && address <= 0xFE9F){
        return oamAccessible() ? oam[address - 0xFE00] : 0xFF;
    }

    // LCD control regs
    switch(address){
        case LCDC_ADDRESS: return LCDC;
        case STAT_ADDRESS: return STAT;  
        case SCY_ADDRESS: return SCY;
        case SCX_ADDRESS: return SCX;
        case LY_ADDRESS: return LY;
        case LYC_ADDRESS: return LYC;
        case BGP_ADDRESS: return BGP;
        case OBP0_ADDRESS: return OBP0;
        case OBP1_ADDRESS: return OBP1;
        case WY_ADDRESS: return WY;
        case WX_ADDRESS: return WX;
        default: return 0xFF;
    }
}

void PPU::write(uint16_t address, uint8_t byte){
    // DMA
    if(address == 0xFF46){
        dmaSource = uint16_t(byte) << 8;
        oamDmaActive = true;
        oamDmaCycles = 160;

        for(int i = 0; i < 0xA0; ++i){
            oam[i] = bus.readDuringDMA(dmaSource + i);
        }

        return;
    }

    // VRAM 8000-9FFF, blocked in mode 3
    if(address >= 0x8000 && address <= 0x9FFF){
        if(vramAccessible()){
            vram[address - 0x8000] = byte;
        }else{
            return;
        }
    }

    // OAM FE00-FE9F, blocked in mode 2 and 3
    if(address >= 0xFE00 && address <= 0xFE9F){
        if(oamAccessible()){
            oam[address - 0xFE00] = byte;
        }else{
            return;
        }
    }

    // LCD control regs
    switch(address){
        case LCDC_ADDRESS: {
            bool wasOn = LCDC & 0x80;
            LCDC = byte;
            bool isOn = LCDC & 0x80;
            if(wasOn && !isOn){
                disableLCD();
            }else if(!wasOn && isOn){
                enableLCD();
            }
        } break;
        case STAT_ADDRESS: STAT = (STAT & 0x87) | (byte & 0x78); break; // only bit 3-6 are writable
        case SCY_ADDRESS: SCY = byte; break;
        case SCX_ADDRESS: SCX = byte; break;
        case LY_ADDRESS: break; // read only
        case LYC_ADDRESS: LYC = byte; break;
        case BGP_ADDRESS: BGP = byte; break;
        case OBP0_ADDRESS: OBP0 = byte; break;
        case OBP1_ADDRESS: OBP1 = byte; break;
        case WY_ADDRESS: WY = byte; break;
        case WX_ADDRESS: WX = byte; break;
        default: break;
    }
}

int PPU::computeObjPenalty(){
    int penalty = 0;
    std::unordered_set<uint16_t> seenTiles;

    struct S{
        int px, index;
        const Sprite* s;
    };

    // sprites sorted from leftmost to rightmost with things with equal values sorted by index
    std::vector<S> sortedSprites;

    for(int i =0; i < static_cast<int>(scanlineSprites.size()); ++i){
        const Sprite& sprite = scanlineSprites[i];
        // gameboy stores sprite X postion in offset of +8 pixels
        sortedSprites.push_back({sprite.x - 8, i, &sprite});
    }

    // stable sort preserves order of values that compare equal, so sorts on index if x is equal
    std::stable_sort(sortedSprites.begin(), sortedSprites.end(), [](const S& a, const S& b){return a.px < b.px;});

    for(S& val : sortedSprites){
        const Sprite& sp = *val.s;
        int px = sp.x - 8;

        if(sp.x == 0){
            // fixed penalty of 11 for completely leftside of the screen sprites
            penalty += 11;
            continue;
        }
        if (px < 0 || px >= 160){
            // penalty only counts for sprites visible in the 0-159 X range ("OBJs overlapping the scanline are considered")
            continue;
        }   

        // fixed 6 dot fetch tile penalty
        penalty += 6;

        // same logic as render fetcher
        bool inWindow = (LCDC & 0x20) && (LY >= WY) && (px >= WX - 7);
        uint16_t mapBase = (!inWindow && (LCDC & 0x08)) ? 0x9C00
                         : (inWindow && (LCDC & 0x40)) ? 0x9C00 : 0x9800;

        int tileCol = inWindow ? (px - (WX - 7)) / 8 : ((SCX + px) / 8) & 0x1F;
        int tileRow = inWindow ? (LY - WY) / 8 : ((LY + SCY) & 0xFF) / 8;

        uint16_t tileAddress = mapBase + tileRow * 32 + tileCol;

        // If havent stalled for this tile add the penalty
        if(!seenTiles.count(tileAddress)){
            seenTiles.insert(tileAddress);
            // gets pixels X inside its 8 pixel tile
            int withinTileX = inWindow ? (px - (WX - 7)) % 8 : (SCX + px) % 8;

            // 7 - withinTile X gets how many pixels are to the right, and then minus 2
            int pen = (7 - withinTileX) - 2;
            if(pen > 0){
                penalty += pen;
            }
        }
    }
    lastMode3Penalty = penalty;
    return penalty;
}

void PPU::renderScanline(){
    if (LY >= 144) {
        // guard to prevent rendering in vblank lines
        std::cerr << "[WARNING] renderScanline() called with LY >= 144 (" << int(LY) << "), skipping.\n";
        return;
    }

    backgroundFIFO.clear();
    spriteFIFO.clear();

    std::vector<SpritePixel> spriteLine(160, {0,false,0});

    int spriteHeight = (LCDC & (1<<2)) ? 16 : 8;

    for(const Sprite& sprite : scanlineSprites){
        int oY = sprite.y;
        int oX = sprite.x;
        int tile = sprite.tileIndex;
        int flags = sprite.flags;

        // which row of this sprite to draw
        int row = LY + 16 - oY;

        if(flags & 0x40){   // y flip flag
            row = spriteHeight - 1 - row;
        }

        // for 8x16 sprites, low bit of tileIndex switches between the two tiles
        if(spriteHeight == 16){
            // force top half tile index to be even
            tile &= 0xFE;
            // if on bottom half of sprite pick the next tile
            if((LY + 16 - oY) >= 8) tile |= 1;
        }

        // get rows 2 bytes from 0x8000
        uint16_t address = 0x8000 + tile*16 + row*2;
        if (address >= 0xA000) {
            std::cerr << "[ERROR] Sprite tile address out-of-bounds: 0x" << std::hex << address << "\n";
            std::exit(1);
        }
        uint8_t low = vramReadRaw(address);
        uint8_t high = vramReadRaw(address + 1);

        bool xFlip = flags & 0x20;
        bool bgPriority = flags & 0x80; // obj to background priority
        uint8_t palette = (flags & 0x10) ? 1 : 0; // OBP1 or OBP0

        for(int bit = 7; bit >= 0; --bit){
            uint8_t colour = ((high >> bit) & 1) << 1 | ((low >> bit) & 1);
            if(colour == 0){
                continue; // transparent
            }

            // compute actual screen X of sprite pixel
            int px = oX - 8 + (xFlip ? (bit) : (7- bit));
            if(px < 0 || px >= 160){
                continue; // off screen pixels
            }

            // when two sprites overlap at the same x the one that was scanned first in mode 2 decides pixel colour
            if(spriteLine[px].colour == 0){
                spriteLine[px] = {colour, bgPriority, palette};
            }
        }
    }

    // helper to push one 8 pixel background/window tile row into the FIFO
    auto pushTileRow = [&](bool window, int xForCalc){
        // LCDC.4=1 0x8000 usigned index, LCDC.4=0 0x8800 signed index
        uint16_t mapBase =
            (!window && (LCDC & 0x08)) ? 0x9C00 :
            ( window && (LCDC & 0x40)) ? 0x9C00 : 0x9800;

        int tileCol = window ? (xForCalc - (WX - 7)) / 8
                                    : ((SCX + xForCalc) / 8) & 0x1F;
        int tileRow = window ? (LY - WY) / 8
                                    : ((LY + SCY) & 0xFF) / 8;

        uint16_t tileMapAddress = mapBase + tileRow * 32 + tileCol;
        uint8_t tileIndex = vramReadRaw(tileMapAddress);

        int fetcherY = window ? (LY - WY) : ((LY + SCY) & 0xFF);
        int fineY = fetcherY % 8;

        uint16_t addressLow;
        if (LCDC & 0x10) {
            addressLow = 0x8000 + uint16_t(tileIndex) * 16 + fineY * 2;
        } else {
            int16_t sIndex = int8_t(tileIndex);
            addressLow = uint16_t(int32_t(0x9000) + sIndex * 16 + fineY * 2);
        }

        uint8_t low  = vramReadRaw(addressLow);
        uint8_t high = vramReadRaw(addressLow + 1);

        for (int bit = 7; bit >= 0; --bit) {
            uint8_t c = (((high >> bit) & 1) << 1) | ((low >> bit) & 1);
            backgroundFIFO.push_back(c);
        }
    };

    // if rendering the background first must drop SCX%8 pixels to account for fine scroll
    bool inWindow0 = ((0 >= (WX - 7)) && (LY >= WY) && (LCDC & 0x20));
    pushTileRow(inWindow0, 0);

    if(!inWindow0){
        int drop = SCX % 8;
        while (drop-- > 0 && !backgroundFIFO.empty()) backgroundFIFO.pop_front();
    }

    bool prevInWindow = inWindow0;

    // iterate over the 160 horizontal pixels
    for(int x = 0; x < 160; ++x){
        bool inWindow = (x >= (WX-7)) && (LY >= WY) && (LCDC & 0x20);

        if(!prevInWindow && inWindow){
            // if just entered window then clear bg fifo
            backgroundFIFO.clear();
        }

        if(backgroundFIFO.size() < 8){
            // refill fifo when low
            pushTileRow(inWindow, x);
        }

        // push sprite pixel
        spriteFIFO.push_back(spriteLine[x]);

        // pixel rendering
        uint8_t bgColour = backgroundFIFO.front();
        backgroundFIFO.pop_front();

        uint8_t objColour = 0;
        bool objHasPriority = false;
        uint8_t objPalette = 0;

        if((LCDC & 0x02) && !spriteFIFO.empty()){   // if obj enable and queue is empty
            SpritePixel sprite = spriteFIFO.front();
            spriteFIFO.pop_front();

            objColour = sprite.colour;
            objPalette = sprite.palette;

            // non transparent and background priority flag
            if(objColour != 0 && !sprite.bgPriority){
                objHasPriority = true;
            }
        }

        // objcolour == 0 then use backg
        // if objs priority flag is clear (obj over backg), it covers background regardless of backgrounds colour
        // if objs priority flag is set (obj behind backg), it only shows when the background pixel is colour 0
        bool spriteCovers = (objColour != 0) && (objHasPriority || (bgColour == 0));

        uint8_t finalIndex = spriteCovers ? objColour : bgColour;

        uint8_t shade;
        if(spriteCovers){
            uint8_t paletteReg = (objPalette == 0) ? OBP0 : OBP1;
            shade = (paletteReg >> (finalIndex*2)) & 0x03;
        }else{
            shade = (BGP >> (finalIndex*2)) & 0x03;
        }

        int fbIndex = LY*160 + x;

        if(fbIndex >= 160 * 144){
            std::cerr << "[ERROR] frameBuffer index out of range: " << fbIndex << "\n";
            std::exit(1);
        }

        frameBuffer[LY*160 + x] = shade;

        prevInWindow = inWindow;
    }
}
