// Animatoon - Axmol 3.0 port
// Small helpers : paths, PureBasic "preference" (ini) files, zip, random.
#pragma once

#include <cstdint>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace ani
{

// ---------------------------------------------------------------- strings / paths
std::string trim(const std::string& s);
std::string toLower(std::string s);
std::vector<std::string> split(const std::string& s, char sep);
/// PureBasic files written on Windows are Latin-1 (or UTF-8 with BOM) : gives valid UTF-8
std::string toUtf8(const std::string& s);
std::string readTextFile(const std::string& path);
bool writeTextFile(const std::string& path, const std::string& text);

std::string pathJoin(const std::string& a, const std::string& b);
std::string pathExt(const std::string& p);      ///< lower case, without the dot
std::string pathFile(const std::string& p);     ///< file name with extension
std::string pathStem(const std::string& p);     ///< file name without extension
std::string pathDir(const std::string& p);
bool fileExists(const std::string& p);
bool dirExists(const std::string& p);
bool makeDirs(const std::string& p);

/// Root directory which contains the "data" folder of Animatoon
const std::string& dataRoot();
void setDataRoot(const std::string& root);
/// Resolve a path relative to the data root. Accepts Windows separators and is not case
/// sensitive (the presets were written on Windows : "data\Presets\Bank\bd\").
std::string resolvePath(const std::string& relative);

struct DirEntry
{
    std::string name;
    bool isDir = false;
};
/// Sorted content of a directory (directories first). Extensions are lower case without dot.
std::vector<DirEntry> listDir(const std::string& dir, const std::vector<std::string>& extensions = {});

// ---------------------------------------------------------------- ini files
/// Reader / writer for the PureBasic preference files (Pref.ini, *.abp, lang *.ini)
class IniFile
{
public:
    bool load(const std::string& path);
    bool save(const std::string& path, const std::string& header = "") const;

    bool has(const std::string& key, const std::string& group = "") const;
    std::string getString(const std::string& key, const std::string& def = "", const std::string& group = "") const;
    int getInt(const std::string& key, int def = 0, const std::string& group = "") const;
    void set(const std::string& key, const std::string& value, const std::string& group = "");
    void set(const std::string& key, int value, const std::string& group = "");

    /// all the key / value of a group, keys keep their original case
    const std::vector<std::pair<std::string, std::string>>& entries(const std::string& group = "") const;

private:
    struct Group
    {
        std::string name;
        std::vector<std::pair<std::string, std::string>> items;
    };
    const Group* find(const std::string& group) const;
    Group* findOrAdd(const std::string& group);
    std::vector<Group> _groups;
};

// ---------------------------------------------------------------- zip (store only writer)
class ZipWriter
{
public:
    void add(const std::string& name, const std::vector<uint8_t>& data);
    void add(const std::string& name, const std::string& text);
    bool save(const std::string& path) const;

private:
    struct Entry
    {
        std::string name;
        std::vector<uint8_t> data;
        uint32_t crc = 0;
    };
    std::vector<Entry> _entries;
};

/// Read every file of a zip archive (name -> content)
bool readZip(const std::string& path, std::map<std::string, std::vector<uint8_t>>& files);

// ---------------------------------------------------------------- random
std::mt19937& rng();
/// Random(max) of PureBasic : integer in 0..max
int rnd(int max);
/// Random(max, min)
int rndRange(int min, int max);
/// Rnd() of the original : Random(a) - Random(a), triangular distribution in -a..a
int rndSym(int a);
float rndf();

} // namespace ani
