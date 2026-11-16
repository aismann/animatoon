// Animatoon - Axmol 3.0 port
#include "Util.h"

#include "axmol/axmol.h"
#if defined(_WIN32)
#    include "ntcvt/ntcvt.hpp"
#endif

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace ani
{

// All the paths of the program are UTF-8 strings
static fs::path toPath(const std::string& s)
{
#if defined(_WIN32)
    return fs::path(ntcvt::from_chars(s));
#else
    return fs::path(s);
#endif
}

static std::string fromPath(const fs::path& p)
{
#if defined(_WIN32)
    return ntcvt::from_chars(p.native());
#else
    return p.string();
#endif
}

// ---------------------------------------------------------------- strings

std::string trim(const std::string& s)
{
    size_t a = 0, b = s.size();
    while (a < b && (unsigned char)s[a] <= ' ')
        ++a;
    while (b > a && (unsigned char)s[b - 1] <= ' ')
        --b;
    return s.substr(a, b - a);
}

std::string toLower(std::string s)
{
    for (auto& c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}

std::vector<std::string> split(const std::string& s, char sep)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s)
    {
        if (c == sep)
        {
            out.push_back(cur);
            cur.clear();
        }
        else
            cur += c;
    }
    out.push_back(cur);
    return out;
}

static bool isValidUtf8(const std::string& s)
{
    size_t i = 0, n = s.size();
    while (i < n)
    {
        unsigned char c = (unsigned char)s[i];
        int extra       = c < 0x80 ? 0 : (c >> 5) == 0x6 ? 1 : (c >> 4) == 0xE ? 2 : (c >> 3) == 0x1E ? 3 : -1;
        if (extra < 0 || i + extra >= n + (extra ? 0 : 1))
            return false;
        for (int k = 1; k <= extra; ++k)
            if (((unsigned char)s[i + k] & 0xC0) != 0x80)
                return false;
        i += extra + 1;
    }
    return true;
}

std::string toUtf8(const std::string& in)
{
    std::string s = in;
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF)
        s.erase(0, 3);
    if (isValidUtf8(s))
        return s;
    std::string out;
    out.reserve(s.size() * 2);
    for (unsigned char c : s)
    {
        if (c < 0x80)
            out += (char)c;
        else
        {
            out += (char)(0xC0 | (c >> 6));
            out += (char)(0x80 | (c & 0x3F));
        }
    }
    return out;
}

std::string readTextFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return toUtf8(ss.str());
}

bool writeTextFile(const std::string& path, const std::string& text)
{
    std::ofstream f(path, std::ios::binary);
    if (!f)
        return false;
    f.write(text.data(), (std::streamsize)text.size());
    return (bool)f;
}

// ---------------------------------------------------------------- paths

static std::string normSlashes(std::string p)
{
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

std::string pathJoin(const std::string& a, const std::string& b)
{
    if (a.empty())
        return normSlashes(b);
    std::string x = normSlashes(a), y = normSlashes(b);
    while (!y.empty() && y.front() == '/')
        y.erase(0, 1);
    if (x.back() != '/')
        x += '/';
    return x + y;
}

std::string pathExt(const std::string& p)
{
    std::string f = pathFile(p);
    size_t d      = f.find_last_of('.');
    return d == std::string::npos ? "" : toLower(f.substr(d + 1));
}

std::string pathFile(const std::string& p)
{
    std::string n = normSlashes(p);
    size_t s      = n.find_last_of('/');
    return s == std::string::npos ? n : n.substr(s + 1);
}

std::string pathStem(const std::string& p)
{
    std::string f = pathFile(p);
    size_t d      = f.find_last_of('.');
    return d == std::string::npos ? f : f.substr(0, d);
}

std::string pathDir(const std::string& p)
{
    std::string n = normSlashes(p);
    size_t s      = n.find_last_of('/');
    return s == std::string::npos ? "" : n.substr(0, s);
}

bool fileExists(const std::string& p)
{
    std::error_code ec;
    return fs::is_regular_file(toPath(p), ec);
}

bool dirExists(const std::string& p)
{
    std::error_code ec;
    return fs::is_directory(toPath(p), ec);
}

bool makeDirs(const std::string& p)
{
    std::error_code ec;
    fs::create_directories(toPath(p), ec);
    return dirExists(p);
}

static std::string s_dataRoot;

const std::string& dataRoot()
{
    return s_dataRoot;
}

void setDataRoot(const std::string& root)
{
    s_dataRoot = normSlashes(root);
    while (s_dataRoot.size() > 1 && s_dataRoot.back() == '/')
        s_dataRoot.pop_back();
}

std::string resolvePath(const std::string& relative)
{
    std::string rel = normSlashes(relative);
    std::error_code ec;
    fs::path p = toPath(rel);
    if (p.is_absolute())
        return rel;
    std::string cur = s_dataRoot;
    for (const auto& part : split(rel, '/'))
    {
        if (part.empty() || part == ".")
            continue;
        std::string direct = pathJoin(cur, part);
        if (fs::exists(toPath(direct), ec))
        {
            cur = direct;
            continue;
        }
        // case insensitive lookup
        std::string want = toLower(part), found;
        if (fs::is_directory(toPath(cur), ec))
        {
            for (auto& e : fs::directory_iterator(toPath(cur), ec))
            {
                std::string n = fromPath(e.path().filename());
                if (toLower(n) == want)
                {
                    found = n;
                    break;
                }
            }
        }
        cur = pathJoin(cur, found.empty() ? part : found);
    }
    return cur;
}

std::vector<DirEntry> listDir(const std::string& dir, const std::vector<std::string>& exts)
{
    std::vector<DirEntry> out;
    std::error_code ec;
    if (!fs::is_directory(toPath(dir), ec))
        return out;
    for (auto& e : fs::directory_iterator(toPath(dir), ec))
    {
        DirEntry d;
        d.name  = fromPath(e.path().filename());
        d.isDir = e.is_directory(ec);
        if (d.name.empty() || d.name[0] == '.')
            continue;
        if (!d.isDir && !exts.empty() && std::find(exts.begin(), exts.end(), pathExt(d.name)) == exts.end())
            continue;
        out.push_back(d);
    }
    std::sort(out.begin(), out.end(), [](const DirEntry& a, const DirEntry& b) {
        if (a.isDir != b.isDir)
            return a.isDir;
        return toLower(a.name) < toLower(b.name);
    });
    return out;
}

// ---------------------------------------------------------------- ini

bool IniFile::load(const std::string& path)
{
    _groups.clear();
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    std::stringstream ss;
    ss << f.rdbuf();
    std::string text = toUtf8(ss.str());
    Group* g         = findOrAdd("");
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == ';')
            continue;
        if (line[0] == '[' && line.back() == ']')
        {
            g = findOrAdd(trim(line.substr(1, line.size() - 2)));
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        g->items.emplace_back(trim(line.substr(0, eq)), trim(line.substr(eq + 1)));
    }
    return true;
}

bool IniFile::save(const std::string& path, const std::string& header) const
{
    std::string out;
    if (!header.empty())
        out += "; " + header + "\n";
    for (auto& g : _groups)
    {
        if (!g.name.empty())
            out += "[" + g.name + "]\n";
        for (auto& kv : g.items)
            out += kv.first + " = " + kv.second + "\n";
    }
    return writeTextFile(path, out);
}

const IniFile::Group* IniFile::find(const std::string& group) const
{
    std::string want = toLower(group);
    for (auto& g : _groups)
        if (toLower(g.name) == want)
            return &g;
    return nullptr;
}

IniFile::Group* IniFile::findOrAdd(const std::string& group)
{
    std::string want = toLower(group);
    for (auto& g : _groups)
        if (toLower(g.name) == want)
            return &g;
    _groups.push_back({group, {}});
    return &_groups.back();
}

bool IniFile::has(const std::string& key, const std::string& group) const
{
    const Group* g = find(group);
    if (!g)
        return false;
    std::string want = toLower(key);
    for (auto& kv : g->items)
        if (toLower(kv.first) == want)
            return true;
    return false;
}

std::string IniFile::getString(const std::string& key, const std::string& def, const std::string& group) const
{
    // PureBasic keeps reading in the current group; our files sometimes hold the key in
    // another group, so fall back to a search in every group.
    std::string want = toLower(key);
    if (const Group* g = find(group))
        for (auto& kv : g->items)
            if (toLower(kv.first) == want)
                return kv.second;
    for (auto& g : _groups)
        for (auto& kv : g.items)
            if (toLower(kv.first) == want)
                return kv.second;
    return def;
}

int IniFile::getInt(const std::string& key, int def, const std::string& group) const
{
    std::string v = getString(key, "", group);
    if (v.empty())
        return def;
    try
    {
        return (int)std::stol(v);
    }
    catch (...)
    {
        return def;
    }
}

void IniFile::set(const std::string& key, const std::string& value, const std::string& group)
{
    Group* g         = findOrAdd(group);
    std::string want = toLower(key);
    for (auto& kv : g->items)
        if (toLower(kv.first) == want)
        {
            kv.second = value;
            return;
        }
    g->items.emplace_back(key, value);
}

void IniFile::set(const std::string& key, int value, const std::string& group)
{
    set(key, std::to_string(value), group);
}

const std::vector<std::pair<std::string, std::string>>& IniFile::entries(const std::string& group) const
{
    static const std::vector<std::pair<std::string, std::string>> empty;
    const Group* g = find(group);
    return g ? g->items : empty;
}

// ---------------------------------------------------------------- zip

static uint32_t crc32of(const uint8_t* data, size_t n)
{
    static uint32_t table[256];
    static bool init = false;
    if (!init)
    {
        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        init = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i)
        c = table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

void ZipWriter::add(const std::string& name, const std::vector<uint8_t>& data)
{
    Entry e;
    e.name = name;
    e.data = data;
    e.crc  = crc32of(data.data(), data.size());
    _entries.push_back(std::move(e));
}

void ZipWriter::add(const std::string& name, const std::string& text)
{
    add(name, std::vector<uint8_t>(text.begin(), text.end()));
}

static void put16(std::string& o, uint16_t v)
{
    o += (char)(v & 0xFF);
    o += (char)(v >> 8);
}

static void put32(std::string& o, uint32_t v)
{
    put16(o, (uint16_t)(v & 0xFFFF));
    put16(o, (uint16_t)(v >> 16));
}

bool ZipWriter::save(const std::string& path) const
{
    // The pictures are already compressed (png) : the entries are simply stored.
    std::string out, central;
    for (auto& e : _entries)
    {
        uint32_t offset = (uint32_t)out.size();
        put32(out, 0x04034b50);
        put16(out, 20), put16(out, 0), put16(out, 0); // version, flags, method = store
        put16(out, 0), put16(out, 0x21);              // time, date (1980-01-01)
        put32(out, e.crc), put32(out, (uint32_t)e.data.size()), put32(out, (uint32_t)e.data.size());
        put16(out, (uint16_t)e.name.size()), put16(out, 0);
        out += e.name;
        out.append((const char*)e.data.data(), e.data.size());

        put32(central, 0x02014b50);
        put16(central, 20), put16(central, 20), put16(central, 0), put16(central, 0);
        put16(central, 0), put16(central, 0x21);
        put32(central, e.crc), put32(central, (uint32_t)e.data.size()), put32(central, (uint32_t)e.data.size());
        put16(central, (uint16_t)e.name.size()), put16(central, 0), put16(central, 0);
        put16(central, 0), put16(central, 0), put32(central, 0);
        put32(central, offset);
        central += e.name;
    }
    uint32_t cdOffset = (uint32_t)out.size();
    out += central;
    put32(out, 0x06054b50);
    put16(out, 0), put16(out, 0);
    put16(out, (uint16_t)_entries.size()), put16(out, (uint16_t)_entries.size());
    put32(out, (uint32_t)central.size()), put32(out, cdOffset);
    put16(out, 0);
    std::ofstream f(path, std::ios::binary);
    if (!f)
        return false;
    f.write(out.data(), (std::streamsize)out.size());
    return (bool)f;
}

bool readZip(const std::string& path, std::map<std::string, std::vector<uint8_t>>& files)
{
    files.clear();
    auto zip = ax::ZipFile::createFromFile(path);
    if (!zip)
        return false;
    for (std::string n = zip->getFirstFilename(); !n.empty(); n = zip->getNextFilename())
    {
        if (n.back() == '/')
            continue;
        std::vector<uint8_t> data;
        ax::ResizableBufferAdapter<std::vector<uint8_t>> buf(&data);
        if (zip->getFileData(n, &buf) && !data.empty())
            files[n] = std::move(data);
    }
    delete zip;
    return !files.empty();
}

// ---------------------------------------------------------------- random

std::mt19937& rng()
{
    static std::mt19937 g{std::random_device{}()};
    return g;
}

int rnd(int max)
{
    if (max <= 0)
        return 0;
    return (int)(rng()() % (uint32_t)(max + 1));
}

int rndRange(int mn, int mx)
{
    if (mx < mn)
        std::swap(mn, mx);
    return mn + rnd(mx - mn);
}

int rndSym(int a)
{
    return rnd(a) - rnd(a);
}

float rndf()
{
    return (rng()() & 0xFFFFFF) / (float)0x1000000;
}

} // namespace ani
