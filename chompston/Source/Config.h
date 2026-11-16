#pragma once
// settings.ini / chomp.cfg handling (replaces the original PureBasic preference files).

#include <string>

struct Settings
{
    // settings.ini (engine / system side)
    int windowWidth          = 768;
    int vsync                = 1;
    int joystickId           = 0;  // stored for compatibility, joystick input is not implemented
    int frameSkip            = 0;
    int storeUserDataLocally = 0;
    int audioOutputBufferSize = 64;  // BASS option, AudioEngine has no equivalent (kept for round-trip)
    int audioUpdatePeriod     = 8;   // BASS option, AudioEngine has no equivalent (kept for round-trip)

    // chomp.cfg (user side)
    int audioVolume = 3;  // 0..kMaxVolume
    int music       = 1;
    int windowed    = 1;
    int smoothing   = 1;
    int hiScore     = 0;

    static constexpr int kMaxVolume = 5;

    float masterVolume() const { return float(audioVolume) / float(kMaxVolume); }

    static Settings& get();

    void load();          // reads settings.ini + chomp.cfg (missing files => defaults)
    void saveUserData();  // writes chomp.cfg
};
