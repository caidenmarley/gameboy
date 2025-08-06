#include "ppu.h"
#include "cpu.h"

int computeObjPenalty(){return 6;}; // TODO object penalty for mode 3

PPU::PPU(Bus& bus) : bus(bus){
    scanlineSprites.reserve(10);
}

void PPU::step(int tStates, CPU& cpu){
    dotCounter += tStates;

    while(true){
        uint8_t mode = STAT & 0x03;
        int needed = 0;

        switch(mode){
            case 2: needed = 80; break;
            case 3: {
                int base = 160; // outputs one pixel to the screen per dot, 160 pixels wide
                int basePenalty = 12; // 12 dot penalty for initial two tile fetches
                int scrollPenalty = SCX % 8; // background scroll penalty
                int windowPenalty = ((LCDC & 0x20) && LY >= WY) ? 6 : 0; // after last non window pxiel a 6 dot penalty is incurred
                int objPenalty = computeObjPenalty();
                needed = base + basePenalty + scrollPenalty + windowPenalty + objPenalty;
            }
            break;
            case 0: {
                int mode3length = 172 + (SCX % 8) + (((LCDC & 0x20) && LY >= WY) ? 6 : 0) + computeObjPenalty();
                needed = 376 - mode3length;
            }
            break;
            case 1: needed = 4560; break;
            default: needed = 0; break;
        }

        if (dotCounter < needed){
            break;
        }

        dotCounter -= needed; // use up the required dots

        switch(mode){
            case 2: {
                scanlineSprites.clear();
                int spriteHeight = (LCDC & (1 << 2)) ? 16 : 8; // LCDC bit 2 determines sprite height
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
                    cpu.requestInterrupt(cpu.Interrupt::LCD);
                }
            }
            break;
            case 0: {

            }
        }
    }
}

uint8_t PPU::read(uint16_t address) const{
    // VRAM 8000-9FFF
    if(address >= 0x8000 && address <= 0x9FFF){
        return vram[address - 0x8000];
    }

    // OAM FE00-FE9F
    if(address >= 0xFE00 && address <= 0xFE9F){
        return oam[address - 0xFE00];
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
    // VRAM 8000-9FFF
    if(address >= 0x8000 && address <= 0x9FFF){
        if(vramAccessible()){
            vram[address - 0x8000] = byte;
        }else{
            return;
        }
    }

    // OAM FE00-FE9F
    if(address >= 0xFE00 && address <= 0xFE9F){
        if(oamAccessible()){
            oam[address - 0xFE00] = byte;
        }else{
            return;
        }
    }

    // LCD control regs
    switch(address){
        case LCDC_ADDRESS: LCDC = byte; break;
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

void PPU::renderScanline(){
    backgroundFIFO.clear();
    spriteFIFO.clear();
    std::vector<SpritePixel> spriteLine;
    spriteLine.resize(160, {0,false,0});

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
        uint8_t low = read(address);
        uint8_t high = read(address + 1);

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


    // iterate over the 160 horizontal pixels
    for(int x = 0; x < 160; ++x){
        // GET TILE
        // screen coords of top left corner of window are (WX-7, WY)
        // LCDC bit 5 controls whether window will be displayed or not
        // LY = 0 is top of screen, larger = further down
        bool inWindow = (x >= (WX - 7)) && (LY >= WY) && (LCDC & 0x20); 
        
        uint16_t mapBase;
        if((LCDC & 0x08) && !inWindow){
            // when LCDC.3 is set and X coord of current scanline is not in window then tilemap 0x9C00 is used
            mapBase = 0x9C00;
        }else if((LCDC & 0x40) && inWindow){
            // when LCDC.6 is set and X coord of current scanline is in window then tilemap 0x9C00 is used
            mapBase = 0x9C00;
        }else{
            mapBase = 0x9800;
        }

        int tileCol, tileRow;
        if(inWindow){
            // use x,y coords for window tile if in window
            tileCol = (x - (WX - 7)) / 8; // how many 8 pixels steps has been moved from window origin
            tileRow = (LY - WY) / 8; // each window is 8 pixels tall
        }else{
            // otherwise X = ((SCX / 8) + fetcher’s X coordinate) & $1F
            // Y = currentScanline + SCY) & 255
            tileCol = ((SCX + x) / 8) & 0x1F;   // equivalent to the above formula
            tileRow = ((LY + SCY) & 0xFF) / 8;
        }

        uint8_t tileIndex = read(mapBase + tileRow * 32 + tileCol); // tile map is 32 tiles wide

        // fetcherY is the row within the tilemap
        int fetcherY = inWindow
                     ? (LY - WY)    // windows vertical offset
                     : ((LY + SCY) & 0xFF); // backgrounds scroll + scanline
        int fineY = fetcherY % 8;   // which row 0-7 of the tile

        // GET TILE DATA LOW
        uint16_t mapBaseLow;
        uint8_t index;
        if(LCDC & 0x10){
            mapBaseLow = 0x8000;
            index = tileIndex;
        }else{
            mapBaseLow = 0x9000;
            index = int8_t(tileIndex);
        }
        uint16_t addressLow = mapBaseLow + index*16 + fineY*2; // each tile = 16 bytes so each row = 2 bytes
        uint8_t lowByte = read(addressLow);
        // GET TILE DATA HIGH 
        uint8_t highByte = read(addressLow + 1);

        // PUSH 8 background pixels
        for(int bit = 7; bit >= 0; --bit){
            uint8_t high = (highByte >> bit) & 1;
            uint8_t low = (lowByte >> bit) & 1;
            // high bit becomes bit 1 of the 2 bit colour and low bit becomes bit 0
            uint8_t colour = (high << 1) | low;
            backgroundFIFO.push_back(colour);
        }

        // PUSH sprite pixel
        spriteFIFO.push_back(spriteLine[x]);

        // PIXEL RENDERING
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

        uint8_t finalIndex;

        if(objHasPriority){
            finalIndex = objColour;
        }else{
            finalIndex = bgColour;
        }

        uint8_t shade;
        if(objHasPriority){
            uint8_t paletteReg = (objPalette == 0) ? OBP0 : OBP1;
            shade = (paletteReg >> (finalIndex*2)) & 0x03;
        }else{
            shade = (BGP >> (finalIndex*2)) & 0x03;
        }

        frameBuffer[LY*160 + x] = shade;
    }
}