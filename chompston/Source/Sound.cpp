#include "Sound.h"

#include "Config.h"
#include "axmol/audio/AudioEngine.h"
#include "axmol/axmol.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

using namespace ax;

namespace
{
constexpr int kRate = 22050;
constexpr float kPi = 3.14159265358979f;

using Wave = std::vector<int16_t>;

float noteHz(int semitonesFromA4) { return 440.f * std::pow(2.f, semitonesFromA4 / 12.f); }

float square(float phase, float duty = 0.5f) { return (phase - std::floor(phase)) < duty ? 1.f : -1.f; }

struct Rng
{
    uint32_t s = 12345;
    float next()
    {
        s = s * 1664525u + 1013904223u;
        return ((s >> 8) & 0xffff) / 32768.f - 1.f;
    }
};

// Adds a tone (square/triangle) with a linear decay envelope to `w` at sample offset `at`.
void tone(Wave& w, float at, float len, float hz0, float hz1, float vol, float duty = 0.5f, bool tri = false)
{
    int start = int(at * kRate), n = int(len * kRate);
    if (start + n > (int)w.size())
        w.resize(start + n, 0);
    float phase = 0;
    for (int i = 0; i < n; ++i)
    {
        float t  = float(i) / n;
        float hz = hz0 + (hz1 - hz0) * t;
        phase += hz / kRate;
        float env = std::min(1.f, i / 60.f) * (1.f - t);
        float v   = tri ? (2.f * std::fabs(2.f * (phase - std::floor(phase + 0.5f))) - 1.f) : square(phase, duty);
        int s     = w[start + i] + int(v * env * vol * 9000);
        w[start + i] = (int16_t)std::clamp(s, -32000, 32000);
    }
}

void noise(Wave& w, float at, float len, float vol, float lowpass)
{
    Rng r;
    int start = int(at * kRate), n = int(len * kRate);
    if (start + n > (int)w.size())
        w.resize(start + n, 0);
    float lp = 0;
    for (int i = 0; i < n; ++i)
    {
        float t = float(i) / n;
        lp += (r.next() - lp) * lowpass;
        int s        = w[start + i] + int(lp * (1.f - t) * (1.f - t) * vol * 16000);
        w[start + i] = (int16_t)std::clamp(s, -32000, 32000);
    }
}

bool writeWav(const std::string& path, const Wave& w)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f)
        return false;
    auto u32 = [&](uint32_t v) { f.write((const char*)&v, 4); };
    auto u16 = [&](uint16_t v) { f.write((const char*)&v, 2); };
    uint32_t dataBytes = uint32_t(w.size() * 2);
    f.write("RIFF", 4);
    u32(36 + dataBytes);
    f.write("WAVEfmt ", 8);
    u32(16);
    u16(1);  // PCM
    u16(1);  // mono
    u32(kRate);
    u32(kRate * 2);
    u16(2);
    u16(16);
    f.write("data", 4);
    u32(dataBytes);
    f.write((const char*)w.data(), dataBytes);
    return (bool)f;
}

Wave makeSfx(Sfx s)
{
    Wave w;
    switch (s)
    {
    case Sfx::Chomp1: tone(w, 0, 0.07f, 330, 520, 0.7f, 0.25f); break;
    case Sfx::Chomp2: tone(w, 0, 0.07f, 392, 620, 0.7f, 0.25f); break;
    case Sfx::Power:
        for (int i = 0; i < 6; ++i)
            tone(w, i * 0.05f, 0.06f, noteHz(-12 + i * 3), noteHz(-9 + i * 3), 0.8f, 0.5f);
        break;
    case Sfx::EatGhost:
        tone(w, 0.00f, 0.08f, noteHz(0), noteHz(4), 0.8f);
        tone(w, 0.08f, 0.08f, noteHz(4), noteHz(7), 0.8f);
        tone(w, 0.16f, 0.14f, noteHz(7), noteHz(12), 0.8f);
        break;
    case Sfx::Die:
        for (int i = 0; i < 14; ++i)
            tone(w, i * 0.07f, 0.08f, 700.f - i * 42.f, 650.f - i * 46.f, 0.8f, 0.5f, i % 2 == 0);
        break;
    case Sfx::Bomb:
        noise(w, 0, 0.7f, 1.0f, 0.12f);
        tone(w, 0, 0.5f, 90, 30, 0.9f, 0.5f, true);
        break;
    case Sfx::Pickup:
        tone(w, 0.00f, 0.07f, noteHz(3), noteHz(3), 0.7f);
        tone(w, 0.07f, 0.07f, noteHz(7), noteHz(7), 0.7f);
        tone(w, 0.14f, 0.07f, noteHz(10), noteHz(10), 0.7f);
        tone(w, 0.21f, 0.14f, noteHz(15), noteHz(15), 0.7f);
        break;
    case Sfx::Start:
    {
        const int n[] = {-9, -5, -2, 3, -2, 3, 7, 10};
        for (int i = 0; i < 8; ++i)
            tone(w, i * 0.14f, 0.13f, noteHz(n[i]), noteHz(n[i]), 0.7f, 0.25f);
        break;
    }
    case Sfx::LevelUp:
    {
        const int n[] = {0, 4, 7, 12, 7, 12, 16};
        for (int i = 0; i < 7; ++i)
            tone(w, i * 0.1f, 0.11f, noteHz(n[i]), noteHz(n[i]), 0.7f, 0.5f);
        break;
    }
    case Sfx::Turn: tone(w, 0, 0.03f, 900, 700, 0.25f, 0.5f); break;
    default: break;
    }
    return w;
}

// 16 beats @ 150 bpm: square-wave bass + pulse lead, loops seamlessly.
Wave makeMusic()
{
    const float beat = 60.f / 150.f;
    // A minor-ish progression (semitones relative to A4)
    const int bass[16] = {-24, -24, -24, -24, -28, -28, -28, -28, -26, -26, -26, -26, -29, -29, -31, -31};
    const int lead[32] = {0, 3, 7, 3, 0, 3, 7, 10, -4, 0, 3, 0, -4, 0, 3, 7,
                          -2, 1, 5, 1, -2, 1, 5, 8,  -5, -2, 2, -2, -5, -2, 2, 5};
    Wave w((size_t)(kRate * beat * 16), 0);
    for (int i = 0; i < 16; ++i)
        tone(w, i * beat, beat * 0.9f, noteHz(bass[i]), noteHz(bass[i]), 0.45f, 0.5f, true);
    for (int i = 0; i < 32; ++i)
        tone(w, i * beat * 0.5f, beat * 0.45f, noteHz(lead[i] + 12), noteHz(lead[i] + 12), 0.22f, 0.25f);
    w.resize((size_t)(kRate * beat * 16));
    return w;
}

const char* kNames[(int)Sfx::Count] = {"chomp1.wav", "chomp2.wav", "power.wav", "eatghost.wav", "die.wav",
                                       "bomb.wav",   "pickup.wav", "start.wav", "levelup.wav", "turn.wav"};
std::string g_dir;
int g_musicId = -1;  // AudioEngine::INVALID_AUDIO_ID

std::string path(const char* n) { return g_dir + n; }
}  // namespace

namespace Sound
{
void init()
{
    auto fu = FileUtils::getInstance();
    g_dir   = fu->getWritablePath() + "chompston_audio/";
    fu->createDirectories(g_dir);

    for (int i = 0; i < (int)Sfx::Count; ++i)
        if (!fu->isFileExist(path(kNames[i])))
            writeWav(path(kNames[i]), makeSfx((Sfx)i));
    if (!fu->isFileExist(path("music.wav")))
        writeWav(path("music.wav"), makeMusic());

    for (int i = 0; i < (int)Sfx::Count; ++i)
        AudioEngine::preload(path(kNames[i]));
}

void play(Sfx s)
{
    auto& st = Settings::get();
    if (st.audioVolume <= 0)
        return;
    AudioEngine::play2d(path(kNames[(int)s]), false, st.masterVolume());
}

void startMusic()
{
    auto& st = Settings::get();
    stopMusic();
    if (!st.music || st.audioVolume <= 0)
        return;
    g_musicId = AudioEngine::play2d(path("music.wav"), true, st.masterVolume() * 0.45f);
}

void stopMusic()
{
    if (g_musicId != AudioEngine::INVALID_AUDIO_ID && g_musicId != -1)
    {
        AudioEngine::stop(g_musicId);
        g_musicId = -1;
    }
}

void applySettings()
{
    auto& st = Settings::get();
    if (!st.music || st.audioVolume <= 0)
        stopMusic();
    else if (g_musicId == -1)
        startMusic();
    else
        AudioEngine::setVolume(g_musicId, st.masterVolume() * 0.45f);
}

void shutdown()
{
    stopMusic();
    AudioEngine::end();
}
}  // namespace Sound
