#include "pb/Platform.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace pb
{

Platform& Platform::get()
{
    static Platform p;
    return p;
}

// ------------------------------------------------------------------ helpers
int rnd(int maxInclusive)
{
    static thread_local std::mt19937 gen{std::random_device{}()};
    if (maxInclusive <= 0)
        return 0;
    return std::uniform_int_distribution<int>(0, maxInclusive)(gen);
}

std::string lcase(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

std::string trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool fileExists(const std::string& path)
{
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

long fileSize(const std::string& path)
{
    std::error_code ec;
    auto sz = fs::file_size(path, ec);
    return ec ? -1 : (long)sz;
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

void makeDirs(const std::string& path)
{
    std::error_code ec;
    fs::create_directories(path, ec);
}

bool writeFile(const std::string& path, const std::string& data)
{
    std::error_code ec;
    fs::path p(path);
    if (p.has_parent_path())
        fs::create_directories(p.parent_path(), ec);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f)
        return false;
    f.write(data.data(), (std::streamsize)data.size());
    return (bool)f;
}

bool removeFile(const std::string& path)
{
    std::error_code ec;
    return fs::remove(path, ec);
}

std::string filePart(const std::string& path)
{
    size_t p = path.find_last_of("/\\");
    return p == std::string::npos ? path : path.substr(p + 1);
}

// ------------------------------------------------------------------ image header sizes
static bool imageSize(const std::string& file, int& w, int& h)
{
    std::ifstream f(file, std::ios::binary);
    if (!f)
        return false;
    unsigned char hdr[32] = {};
    f.read((char*)hdr, 32);
    if (!std::memcmp(hdr, "\x89PNG", 4))
    {
        w = (hdr[16] << 24) | (hdr[17] << 16) | (hdr[18] << 8) | hdr[19];
        h = (hdr[20] << 24) | (hdr[21] << 16) | (hdr[22] << 8) | hdr[23];
        return true;
    }
    if (hdr[0] == 0xFF && hdr[1] == 0xD8)  // JPEG: walk segments to SOFn
    {
        f.clear();
        f.seekg(2);
        for (;;)
        {
            int c = f.get();
            if (c == EOF)
                return false;
            if (c != 0xFF)
                continue;
            int marker = f.get();
            while (marker == 0xFF)
                marker = f.get();
            if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7))
                continue;
            int len = (f.get() << 8) | f.get();
            if ((marker >= 0xC0 && marker <= 0xCF) && marker != 0xC4 && marker != 0xC8 && marker != 0xCC)
            {
                f.get();  // precision
                h = (f.get() << 8) | f.get();
                w = (f.get() << 8) | f.get();
                return true;
            }
            f.seekg(len - 2, std::ios::cur);
        }
    }
    return false;
}

// ------------------------------------------------------------------ sheets
int Platform::loadSheet(const std::string& relPath)
{
    std::lock_guard<std::mutex> lk(_mtx);
    for (size_t i = 0; i < _sheets.size(); ++i)
        if (_sheets[i].path == relPath)
            return (int)i;
    Sheet s;
    s.path = relPath;
    imageSize(_resRoot + "/" + relPath, s.w, s.h);
    _sheets.push_back(s);
    return (int)_sheets.size() - 1;
}

int Platform::sheetCount() const
{
    std::lock_guard<std::mutex> lk(_mtx);
    return (int)_sheets.size();
}

// ------------------------------------------------------------------ fonts
void Platform::setGlyphWidths(int font, const std::vector<float>& widths, float lineHeight)
{
    _glyph[font] = widths;
    _lineH[font] = lineHeight;
}

float Platform::textWidth(const std::string& s) const
{
    const auto& g = _glyph[_font];
    float w = 0;
    for (size_t i = 0; i < s.size();)
    {
        unsigned char c = (unsigned char)s[i];
        unsigned cp = c;
        size_t len = 1;
        if (c >= 0xF0 && i + 3 < s.size())
            cp = ((c & 7) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F), len = 4;
        else if (c >= 0xE0 && i + 2 < s.size())
            cp = ((c & 15) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F), len = 3;
        else if (c >= 0xC0 && i + 1 < s.size())
            cp = ((c & 31) << 6) | (s[i + 1] & 0x3F), len = 2;
        w += (cp < g.size()) ? g[cp] : 8.0f;
        i += len;
    }
    return w;
}

float Platform::textHeight(const std::string&) const
{
    return _lineH[_font] > 0 ? _lineH[_font] : 16.0f;
}

// ------------------------------------------------------------------ drawing (game thread)
void Platform::clearScreen(uint32_t color)
{
    _back.clear();
    _backClear = color;
}

void Platform::blit(int sheet, int sx, int sy, int sw, int sh, float x, float y, int alpha, bool opaque, float dw,
                    float dh)
{
    Cmd c;
    c.type = Cmd::Blit;
    c.sheet = sheet;
    c.sx = sx;
    c.sy = sy;
    c.sw = sw;
    c.sh = sh;
    c.x = x;
    c.y = y;
    c.alpha = (uint8_t)std::clamp(alpha, 0, 255);
    c.opaque = opaque;
    c.w = dw;
    c.h = dh;
    _back.push_back(std::move(c));
}

void Platform::blitFull(int sheet, float x, float y, int alpha, bool opaque)
{
    const Sheet& s = _sheets[sheet];
    blit(sheet, 0, 0, s.w, s.h, x, y, alpha, opaque);
}

void Platform::box(float x, float y, float w, float h, uint32_t color, int alpha)
{
    Cmd c;
    c.type = Cmd::Box;
    c.x = x;
    c.y = y;
    c.w = w;
    c.h = h;
    c.color = color;
    c.alpha = (uint8_t)std::clamp(alpha, 0, 255);
    _back.push_back(std::move(c));
}

void Platform::line(float x1, float y1, float x2, float y2, uint32_t color)
{
    Cmd c;
    c.type = Cmd::Line;
    c.x = x1;
    c.y = y1;
    c.w = x2;
    c.h = y2;
    c.color = color;
    _back.push_back(std::move(c));
}

void Platform::text(float x, float y, const std::string& s, uint32_t fg, uint32_t bg, int alpha)
{
    if (s.empty())
        return;
    if (bg != 0)
        box(x, y, textWidth(s), textHeight(), bg);
    Cmd c;
    c.type = Cmd::Text;
    c.x = x;
    c.y = y;
    c.text = s;
    c.color = fg;
    c.font = (uint8_t)_font;
    c.alpha = (uint8_t)std::clamp(alpha, 0, 255);
    _back.push_back(std::move(c));
}

void Platform::flip()
{
    std::unique_lock<std::mutex> lk(_mtx);
    checkQuit();
    _front = _back;  // keep _back: drawing persists until clearScreen()
    _frontClear = _backClear;
    _frameReady = true;
    uint64_t target = ++_framesPublished;
    _cv.wait(lk, [&] { return _framesPresented >= target || _quit; });
    checkQuit();
}

void Platform::delayMs(int ms)
{
    auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (std::chrono::steady_clock::now() < end)
    {
        checkQuit();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    checkQuit();
}

uint32_t Platform::elapsedMs() const
{
    static auto t0 = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
}

void Platform::requestScreenshot(const std::string& relPathWritable)
{
    std::lock_guard<std::mutex> lk(_mtx);
    _screenshot = relPathWritable;
}

// ------------------------------------------------------------------ input
void Platform::examineKeyboard()
{
    std::lock_guard<std::mutex> lk(_mtx);
    checkQuit();
    std::memcpy(_rel, _relAccum, sizeof(_rel));
    std::memset(_relAccum, 0, sizeof(_relAccum));
}

bool Platform::keyPushed(int key) const
{
    std::lock_guard<std::mutex> lk(_mtx);
    if (key == K_ALL)
    {
        for (int i = 1; i < K_ALL; ++i)
            if (_down[i])
                return true;
        return false;
    }
    return key > 0 && key < K_COUNT && _down[key];
}

bool Platform::keyReleased(int key) const
{
    std::lock_guard<std::mutex> lk(_mtx);
    return key > 0 && key < K_COUNT && _rel[key];
}

void Platform::examineMouse() {}

int Platform::mouseWheel()
{
    std::lock_guard<std::mutex> lk(_mtx);
    int w = _wheelAccum;
    _wheelAccum = 0;
    return w;
}

// ------------------------------------------------------------------ sound
void Platform::playSound(const std::string& resRelPath, float volume)
{
    std::lock_guard<std::mutex> lk(_mtx);
    _sounds.emplace_back(resRelPath, volume);
}

std::vector<std::pair<std::string, float>> Platform::takeSounds()
{
    std::lock_guard<std::mutex> lk(_mtx);
    auto v = std::move(_sounds);
    _sounds.clear();
    return v;
}

// ------------------------------------------------------------------ main-thread side
bool Platform::takeFrame(std::vector<Cmd>& out, uint32_t& clearColor)
{
    std::lock_guard<std::mutex> lk(_mtx);
    if (!_frameReady)
        return false;
    out.swap(_front);
    clearColor = _frontClear;
    _frameReady = false;
    return true;
}

void Platform::signalFrameDone()
{
    {
        std::lock_guard<std::mutex> lk(_mtx);
        _framesPresented = _framesPublished;
    }
    _cv.notify_all();
}

void Platform::onKey(int key, bool down)
{
    if (key <= 0 || key >= K_ALL)
        return;
    std::lock_guard<std::mutex> lk(_mtx);
    if (!down && _down[key])
    {
        _relAccum[key] = true;
        _relAccum[K_ALL] = true;
    }
    _down[key] = down;
}

void Platform::onMouse(int x, int y, bool left)
{
    if (left && !_mouseLeft)
        _clicked = true;
    _mouseX = x;
    _mouseY = y;
    _mouseLeft = left;
}

void Platform::onWheel(int delta)
{
    std::lock_guard<std::mutex> lk(_mtx);
    _wheelAccum += delta;
}

void Platform::requestQuit()
{
    _quit = true;
    _cv.notify_all();
}

bool Platform::takeScreenshotRequest(std::string& out)
{
    std::lock_guard<std::mutex> lk(_mtx);
    if (_screenshot.empty())
        return false;
    out = std::move(_screenshot);
    _screenshot.clear();
    return true;
}

}  // namespace pb
