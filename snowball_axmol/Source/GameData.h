#pragma once
#include <cstdint>

// Raw data of the original game (see tools/extract_assets.py)
namespace gamedata
{
extern const uint8_t palette[64];      // 16 x 32 bit colours, memory order R,G,B,0 (PureBasic RGB())
extern const uint8_t sprites[13 * 64]; // 13 C64 multicolor sprites, 24x21, 63 bytes + 1 pad
extern const uint8_t chars[256 * 8];   // C64 multicolor charset, 8 bytes per character
extern const uint8_t charColors[256];  // colour attribute per character (value - 8 = palette index)
extern const uint8_t map[25 * 80];     // tile map, 80 x 25 tiles (0 = empty)
extern const uint8_t anims[6 * 16];    // per state: [0]=ticks per frame, [1..]=frame numbers, 0-terminated
}  // namespace gamedata
