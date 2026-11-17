// Lost Labyrinth: Portals - Axmol port
// Thin "PureBasic-like" platform layer: the original game logic runs on its own
// thread with blocking loops (FlipBuffers / Delay / ExamineKeyboard); this layer
// hands finished frames over to the Axmol render thread and feeds input back.
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace pb
{

// thrown inside the game thread when the window is closed
struct QuitException
{};

// ---------------------------------------------------------------- colours
// PureBasic packs colours as 0x00BBGGRR
constexpr uint32_t rgb(int r, int g, int b)
{
    return (uint32_t)(r & 255) | ((uint32_t)(g & 255) << 8) | ((uint32_t)(b & 255) << 16);
}

// ---------------------------------------------------------------- fonts
enum Font
{
    FONT_DEFAULT = 0,
    FONT_SMALL,
    FONT_HARRINGTON,
    FONT_ARIAL_NORMAL,
    FONT_VERDANA,
    FONT_VERDANA_BOLD,
    FONT_COUNT
};

// ---------------------------------------------------------------- keys
// subset of PureBasic keycodes that the game uses; values are our own indices
enum Key
{
    K_NONE = 0,
    K_ESCAPE,
    K_RETURN,
    K_SPACE,
    K_UP,
    K_DOWN,
    K_LEFT,
    K_RIGHT,
    K_LEFT_ALT,
    K_PAGE_UP,
    K_PAGE_DOWN,
    K_F1,
    K_BACKSPACE,
    K_DELETE,
    K_TAB,
    K_0,
    K_A = K_0 + 10,  // K_A .. K_Z follow
    K_Z = K_A + 25,
    K_ALL = 200,  // pseudo key: "any key"
    K_COUNT = 201
};

// ---------------------------------------------------------------- draw commands
struct Cmd
{
    enum Type : uint8_t
    {
        Blit,
        Box,
        Line,
        Text
    };
    Type type = Box;
    bool opaque = false;  // blit: ignore alpha channel of the image
    uint8_t alpha = 255;
    uint8_t font = 0;
    int sheet = -1;
    int sx = 0, sy = 0, sw = 0, sh = 0;  // source rect (blit)
    float x = 0, y = 0, w = 0, h = 0;    // destination
    uint32_t color = 0, bg = 0;
    std::string text;
};

struct Sheet
{
    std::string path;  // relative to the resource root
    int w = 0, h = 0;
};

// ---------------------------------------------------------------- state shared by both threads
class Platform
{
public:
    static Platform& get();

    // ---- setup (main thread, before the game thread starts)
    void setResourceRoot(const std::string& root) { _resRoot = root; }
    void setWritableRoot(const std::string& root) { _writeRoot = root; }
    const std::string& resRoot() const { return _resRoot; }
    const std::string& writeRoot() const { return _writeRoot; }
    void setGlyphWidths(int font, const std::vector<float>& widths, float lineHeight);

    // ---- sheets (images)
    int loadSheet(const std::string& relPath);  // dedups, reads image size from header
    const Sheet& sheet(int id) const { return _sheets[id]; }
    int sheetCount() const;

    // ---- game thread: drawing
    void clearScreen(uint32_t color = 0);
    void blit(int sheet, int sx, int sy, int sw, int sh, float x, float y, int alpha = 255, bool opaque = false,
              float dw = 0, float dh = 0);
    void blitFull(int sheet, float x, float y, int alpha = 255, bool opaque = false);
    void box(float x, float y, float w, float h, uint32_t color, int alpha = 255);
    void line(float x1, float y1, float x2, float y2, uint32_t color);
    void text(float x, float y, const std::string& s, uint32_t fg, uint32_t bg = 0, int alpha = 255);
    void setFont(int font) { _font = font; }
    float textWidth(const std::string& s) const;
    float textHeight(const std::string& s = "") const;
    void flip();                // publish frame, wait until it has been presented
    void delayMs(int ms);       // interruptible sleep
    void requestScreenshot(const std::string& relPathWritable);

    // ---- game thread: input
    void examineKeyboard();
    bool keyPushed(int key) const;
    bool keyReleased(int key) const;  // released since last examineKeyboard(); K_ALL = any key
    int  mouseX() const { return _mouseX; }
    int  mouseY() const { return _mouseY; }
    bool mouseLeft() const { return _mouseLeft; }
    bool takeClick()  // left button went down since last call
    {
        bool c = _clicked;
        _clicked = false;
        return c;
    }
    void examineMouse();
    int  mouseWheel();
    uint32_t elapsedMs() const;

    // ---- sound (game thread posts, main thread plays)
    void playSound(const std::string& resRelPath, float volume);
    std::vector<std::pair<std::string, float>> takeSounds();

    // ---- main thread: renderer side
    bool takeFrame(std::vector<Cmd>& out, uint32_t& clearColor);  // false if nothing new
    void onKey(int key, bool down);
    void onMouse(int x, int y, bool left);
    void onWheel(int delta);
    void requestQuit();
    bool quitting() const { return _quit; }
    bool takeScreenshotRequest(std::string& out);
    void signalFrameDone();

private:
    Platform() = default;
    void checkQuit() const
    {
        if (_quit)
            throw QuitException{};
    }

    std::string _resRoot, _writeRoot;

    mutable std::mutex _mtx;
    std::condition_variable _cv;
    std::atomic<bool> _quit{false};

    std::vector<Sheet> _sheets;

    // double buffer
    std::vector<Cmd> _back, _front;
    uint32_t _backClear = 0, _frontClear = 0;
    bool _frameReady = false;
    uint64_t _framesPresented = 0, _framesPublished = 0;

    int _font = 0;
    std::vector<float> _glyph[FONT_COUNT];
    float _lineH[FONT_COUNT] = {};

    // input
    bool _down[K_COUNT] = {};
    bool _relAccum[K_COUNT] = {};
    bool _rel[K_COUNT] = {};
    volatile int _mouseX = 0, _mouseY = 0;
    volatile bool _mouseLeft = false;
    volatile bool _clicked = false;
    int _wheelAccum = 0;

    std::vector<std::pair<std::string, float>> _sounds;
    std::string _screenshot;
};

// ---------------------------------------------------------------- small helpers (PureBasic parity)
int rnd(int maxInclusive);                    // Random(n): 0..n inclusive
inline int imin(int a, int b) { return a < b ? a : b; }
inline int imax(int a, int b) { return a > b ? a : b; }

std::string lcase(std::string s);
std::string trim(const std::string& s);
bool fileExists(const std::string& path);
long fileSize(const std::string& path);
bool readFile(const std::string& path, std::string& out);
bool writeFile(const std::string& path, const std::string& data);
void makeDirs(const std::string& path);
bool removeFile(const std::string& path);
std::string filePart(const std::string& path);  // GetFilePart, accepts both separators

}  // namespace pb
