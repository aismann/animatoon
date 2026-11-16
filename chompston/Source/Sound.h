#pragma once
// Audio replacement for the original BASS.dll code: all sounds are synthesised at first start
// (chiptune style, 16 bit mono WAV) and played back through ax::AudioEngine.

#include <string>

enum class Sfx
{
    Chomp1,
    Chomp2,
    Power,
    EatGhost,
    Die,
    Bomb,
    Pickup,
    Start,
    LevelUp,
    Turn,
    Count
};

namespace Sound
{
void init();                 // generate files (if missing) and preload
void play(Sfx s);
void startMusic();
void stopMusic();
void applySettings();        // volume / music on-off from Settings
void shutdown();
}  // namespace Sound
