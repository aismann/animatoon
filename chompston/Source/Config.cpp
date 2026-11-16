#include "Config.h"

#include "axmol/axmol.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>

using namespace ax;

namespace
{
std::string trim(std::string s)
{
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

std::map<std::string, std::string> parseIni(const std::string& text)
{
    std::map<std::string, std::string> kv;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '[')
            continue;
        auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        auto key = trim(line.substr(0, eq));
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
        kv[key] = trim(line.substr(eq + 1));
    }
    return kv;
}

bool readFile(const std::string& path, std::string& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

// Looks next to the executable / in Content/ / in the user's writable folder.
bool readFirst(const std::vector<std::string>& candidates, std::string& out)
{
    for (auto& c : candidates)
        if (!c.empty() && readFile(c, out))
            return true;
    return false;
}

int geti(const std::map<std::string, std::string>& kv, const char* key, int def)
{
    auto it = kv.find(key);
    if (it == kv.end())
        return def;
    try
    {
        return std::stoi(it->second);
    }
    catch (...)
    {
        return def;
    }
}

std::string userFilePath(const Settings& s)
{
    auto fu = FileUtils::getInstance();
    if (s.storeUserDataLocally)
        return "chomp.cfg";  // current working directory (the game folder)
    return fu->getWritablePath() + "chomp.cfg";
}
}  // namespace

Settings& Settings::get()
{
    static Settings s;
    return s;
}

void Settings::load()
{
    auto fu = FileUtils::getInstance();
    std::string text;

    if (readFirst({"settings.ini", "Content/settings.ini", fu->fullPathForFilename("settings.ini")}, text))
    {
        auto kv              = parseIni(text);
        windowWidth          = std::clamp(geti(kv, "windowwidth", windowWidth), 256, 4096);
        vsync                = geti(kv, "vsync", vsync) != 0;
        joystickId           = geti(kv, "joystickid", joystickId);
        frameSkip            = std::clamp(geti(kv, "frameskip", frameSkip), 0, 10);
        storeUserDataLocally = geti(kv, "storeuserdatalocally", storeUserDataLocally) != 0;
        audioOutputBufferSize = geti(kv, "audiooutputbuffersize", audioOutputBufferSize);
        audioUpdatePeriod     = geti(kv, "audioupdateperiod", audioUpdatePeriod);
    }

    text.clear();
    // user data wins over the shipped defaults
    if (readFirst({userFilePath(*this), "chomp.cfg", "Content/chomp.cfg", fu->fullPathForFilename("chomp.cfg")},
                  text))
    {
        auto kv     = parseIni(text);
        audioVolume = std::clamp(geti(kv, "audiovolume", audioVolume), 0, kMaxVolume);
        music       = geti(kv, "music", music) != 0;
        windowed    = geti(kv, "windowed", windowed) != 0;
        smoothing   = geti(kv, "smoothing", smoothing) != 0;
        hiScore     = std::max(0, geti(kv, "hiscore", hiScore));
    }
}

void Settings::saveUserData()
{
    std::ofstream f(userFilePath(*this), std::ios::binary | std::ios::trunc);
    if (!f)
        return;
    f << "AudioVolume = " << audioVolume << "\n"
      << "Music = " << music << "\n"
      << "Windowed = " << windowed << "\n"
      << "Smoothing = " << smoothing << "\n"
      << "HiScore = " << hiScore << "\n";
}
