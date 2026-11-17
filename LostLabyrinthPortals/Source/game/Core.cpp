// misc.pb, preferences.pb, messages.pb, sound.pb and map_handling.pb
#include "game/Game.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <set>

namespace ll
{

using pb::str;
using pb::val;
using pb::XmlDoc;
using pb::XmlNode;

// ================================================================ globals
const char* version_number = "3.0.2";
std::string program_title = std::string("Lost Labyrinth: Portals ") + version_number;
char_struct current_character;
map_struct current_map;
preferences_struct preferences;
message_struct message_;
std::list<map_event_struct> map_event;
std::list<map_transformation_struct> map_transformation;
std::vector<ability_db_struct> ability_db(MAX_NUMBER_OF_ABILITIES);
int program_ends = 0;
std::list<monster_struct> monster;
std::vector<monster_type_db_struct> monster_type_db(MAX_NUMBER_OF_MONSTER_TYPES);
std::vector<key_binding_struct> key_binding(MAX_NUMBER_OF_ACTIONS);
std::vector<power_db_struct> power_db(MAX_NUMBER_OF_POWERS);
std::list<item_in_inventory_struct> inventory;
std::vector<item_db_struct> item_db(MAX_NUMBER_OF_ITEMS);
std::list<item_on_map_struct> item_on_map;
std::list<dungeon_struct> dungeon_on_map;
std::vector<std::string> message_list(MAX_NUMBER_OF_MESSAGES + 1);
int silhouette = 0;
int startLastPoint = 1;
int item_identification_global = 1;
int wWidth = 640, wHeight = 480;
int sheets[SPRITE_COUNT];
int tile_columns[SPRITE_COUNT];

static std::set<std::string> g_sounds;

// ================================================================ paths / misc
std::string res_path(const std::string& rel)
{
    return P().resRoot() + "/" + rel;
}

std::string save_path(const std::string& rel)
{
    return P().writeRoot() + "/savegame/" + rel;
}

std::string file_part(const std::string& p)
{
    return pb::filePart(p);
}

void error_message(const std::string& msg)
{
    // the original showed a requester and ended the program
    fprintf(stderr, "[Lost Labyrinth] ERROR: %s\n", msg.c_str());
    program_ends = 1;
    P().requestQuit();
    throw pb::QuitException{};
}

bool compare_string_lists(const std::string& a, const std::string& b, char sep)
{
    auto split = [&](const std::string& s)
    {
        std::vector<std::string> v;
        std::string cur;
        for (char c : s)
        {
            if (c == sep)
            {
                v.push_back(pb::lcase(cur));
                cur.clear();
            }
            else
                cur += c;
        }
        if (!cur.empty())
            v.push_back(pb::lcase(cur));
        return v;
    };
    auto va = split(a), vb = split(b);
    for (auto& x : va)
        for (auto& y : vb)
            if (x == y)
                return true;
    return false;
}

std::string add_string_list(std::string list, const std::string& add, char sep)
{
    if (!compare_string_lists(list, add, sep))
    {
        if (!list.empty())
            list += sep;
        list += add;
    }
    return list;
}

std::string replace_string(std::string s, const std::string& what, const std::string& with, bool nocase)
{
    if (what.empty())
        return s;
    std::string hay = nocase ? pb::lcase(s) : s;
    std::string needle = nocase ? pb::lcase(what) : what;
    std::string out;
    size_t pos = 0;
    for (;;)
    {
        size_t f = hay.find(needle, pos);
        if (f == std::string::npos)
        {
            out += s.substr(pos);
            break;
        }
        out += s.substr(pos, f - pos);
        out += with;
        pos = f + needle.size();
    }
    return out;
}

std::string yes_no_attr(const XmlNode& n, const char* a)
{
    return n.attrOr(a);
}

// ================================================================ preferences
void reset_key_bindings()
{
    key_binding[ACTION_TEST] = {"test", pb::K_A + ('T' - 'A')};
    key_binding[ACTION_ESCAPE] = {"escape", pb::K_ESCAPE};
    key_binding[ACTION_UP] = {"move up", pb::K_UP};
    key_binding[ACTION_DOWN] = {"move down", pb::K_DOWN};
    key_binding[ACTION_LEFT] = {"move left", pb::K_LEFT};
    key_binding[ACTION_RIGHT] = {"move right", pb::K_RIGHT};
}

void load_preferences()
{
    preferences = preferences_struct{};
    reset_key_bindings();
    XmlDoc d;
    // user preferences live in the writable dir, defaults ship with the game
    std::string path = P().writeRoot() + "/config/preferences.xml";
    if (!pb::fileExists(path))
        path = res_path("config/preferences.xml");
    if (!d.load(path))
        return;
    for (auto& a : d.root->attrs)
    {
        if (a.first == "fullscreen")
            preferences.fullscreen = a.second == "yes";
        else if (a.first == "message_delay")
            preferences.message_delay = (int)val(a.second);
        else if (a.first == "DDD_effects")
            preferences.DDD_effects = a.second == "yes";
        else if (a.first == "fade_effect")
            preferences.fade_effect = a.second == "yes";
        else if (a.first == "language")
            preferences.language = a.second;
    }
}

void save_preferences()
{
    XmlDoc d;
    d.root = std::make_unique<XmlNode>();
    d.root->name = "Lost_Labyrinth_Preferences";
    d.root->set("Lost_Labyrinth_version", version_number);
    d.root->set("fullscreen", preferences.fullscreen ? "yes" : "no");
    d.root->set("message_delay", str(preferences.message_delay));
    d.root->set("DDD_effects", preferences.DDD_effects ? "yes" : "no");
    d.root->set("fade_effect", preferences.fade_effect ? "yes" : "no");
    d.root->set("language", preferences.language);
    d.save(P().writeRoot() + "/config/preferences.xml");
}

// ================================================================ messages
void load_message_list()
{
    for (int i = 1; i < MAX_NUMBER_OF_MESSAGES; ++i)
        message_list[i] = "??? missing message ID " + str(i) + " ???";
    XmlDoc d;
    if (!d.load(res_path("data/messages.xml")))
        error_message("load_message_list(): could not open data/messages.xml: " + d.error);
    for (auto& m : d.root->children)
    {
        if (m->name != "message")
            continue;
        int id = (int)val(m->attrOr("id"));
        if (id < 1 || id > MAX_NUMBER_OF_MESSAGES - 1)
            continue;
        for (auto& t : m->children)
            if (t->name == "text" && t->attrOr("language") == preferences.language)
                message_list[id] = t->text;
    }
}

// ================================================================ sound
void load_sound()
{
    namespace fs = std::filesystem;
    std::error_code ec;
    for (auto& e : fs::directory_iterator(res_path("sound"), ec))
        if (e.path().extension() == ".ogg")
            g_sounds.insert(e.path().stem().string());
}

void play_sound(const std::string& name)
{
    if (name.empty() || !g_sounds.count(name))
        return;
    P().playSound("sound/" + name + ".ogg", 0.5f);
}

// ================================================================ tilesets
void reset_tile_type_db(tileset_struct& ts)
{
    ts.tile_type_db.assign(MAX_NUMBER_OF_TILE_TYPES, tile_type_db_struct{});
}

static void resolve_tile_type_definition_names(tileset_struct& ts)
{
    for (auto& t : ts.tile_type_db)
        for (auto& m : t.modify_attribute)
        {
            for (int k = 1; k < MAX_NUMBER_OF_ABILITIES; ++k)
            {
                if (!m.attribute_name.empty() && ability_db[k].name == m.attribute_name)
                    m.attribute = k;
                if (!m.amount_reduced_by_ability_name.empty() && ability_db[k].name == m.amount_reduced_by_ability_name)
                    m.amount_reduced_by_ability = k;
            }
        }
}

void reset_tileset(tileset_struct& ts)
{
    ts.name.clear();
    ts.image_filename.clear();
    ts.definition_filename.clear();
    ts.sheet = -1;
    ts.rows = ts.columns = 0;
    reset_tile_type_db(ts);
}

void load_tile_type_definition(tileset_struct& ts)
{
    std::string path = res_path("tilesets/" + file_part(ts.definition_filename));
    XmlDoc d;
    if (!d.load(path))
        error_message("load_tile_type_definition(): could not open " + path + ": " + d.error);
    reset_tile_type_db(ts);
    for (auto& n : d.root->children)
    {
        if (n->name != "tile_type")
            continue;
        int id = -1;
        tile_type_db_struct t;
        for (auto& a : n->attrs)
        {
            const std::string& k = a.first;
            const std::string& v = a.second;
            if (k == "id")
                id = (int)val(v);
            else if (k == "solid")
                t.solid = v == "yes";
            else if (k == "blocks_flight")
                t.blocks_flight = v == "yes";
            else if (k == "name")
                t.name = v;
            else if (k == "transparent")
                t.transparent = v == "yes";
            else if (k == "message")
                t.message = v;
            else if (k == "remove_after_entering")
                t.remove_after_entering = v == "yes";
            else if (k == "sound")
                t.sound = v;
            else if (k == "transforms_into")
                t.transforms_into = (int)val(v);
            else if (k == "class")
                t.class_ = v;
            else if (k == "container")
                t.container = (int)val(v);
            else if (k == "hidden")
                t.hidden = v == "yes";
            else if (k == "detection_message")
                t.detection_message = v;
            else if (k == "detection_sound")
                t.detection_sound = v;
            else if (k == "animation_enter")
                t.animation_enter = v;
        }
        if (id < 0 || id >= MAX_NUMBER_OF_TILE_TYPES)
            continue;
        t.in_use = 1;
        int mod_idx = 0;
        for (auto& c : n->children)
        {
            bool lang_ok = c->attrOr("language") == preferences.language;
            if (c->name == "name")
            {
                if (lang_ok)
                    t.name = c->text;
            }
            else if (c->name == "message")
            {
                if (lang_ok)
                    t.message = c->text;
            }
            else if (c->name == "detection_message")
            {
                if (lang_ok)
                    t.detection_message = c->text;
            }
            else if (c->name == "modify_attribute")
            {
                modify_attribute_by_tile_struct m;
                for (auto& a : c->attrs)
                {
                    const std::string& k = a.first;
                    const std::string& v = a.second;
                    if (k == "attribute")
                        m.attribute = (int)val(v);
                    else if (k == "attribute_name")
                        m.attribute_name = v;
                    else if (k == "amount")
                        m.amount = (int)val(v);
                    else if (k == "amount_randomized")
                        m.amount_randomized = v == "yes";
                    else if (k == "amount_reduced_by_ability_name")
                        m.amount_reduced_by_ability_name = v;
                    else if (k == "replenish")
                        m.replenish = v == "yes";
                    else if (k == "trigger_hidden")
                        m.trigger_hidden = v == "yes";
                    else if (k == "turn_ends")
                        m.turn_ends = v == "yes";
                }
                if ((m.attribute > 0 || !m.attribute_name.empty()) && mod_idx < MAX_NUMBER_OF_ATTRIBUTES_MODIFIED_BY_TILE)
                    t.modify_attribute[mod_idx++] = m;
            }
        }
        ts.tile_type_db[id] = t;
    }
    resolve_tile_type_definition_names(ts);
}

void load_tileset(tileset_struct& ts)
{
    std::string rel = "tilesets/" + file_part(ts.image_filename);
    if (pb::fileSize(res_path(rel)) < 0)
        error_message("load_tileset: cannot find tileset image file " + rel);
    ts.sheet = P().loadSheet(rel);
    ts.columns = P().sheet(ts.sheet).w / 32;
    ts.rows = P().sheet(ts.sheet).h / 32;
    load_tile_type_definition(ts);
}

// ================================================================ map field access
static bool in_map(int x, int y)
{
    return x >= 0 && x < MAP_DIMENSION_X && y >= 0 && y < MAP_DIMENSION_Y;
}
static mapfield_struct& fld(map_struct& m, int x, int y)
{
    return m.mapfield[x + y * MAP_DIMENSION_Y];
}
static const mapfield_struct& fld(const map_struct& m, int x, int y)
{
    return m.mapfield[x + y * MAP_DIMENSION_Y];
}

int read_map_tile_type(const map_struct& m, int x, int y)
{
    if (!in_map(x, y))
        return m.tile_outside_map;
    return fld(m, x, y).tile_type;
}

bool random_field(map_struct& m, coordinates_struct& c, int tile_type, int limit_x, int limit_y)
{
    if (tile_type == -1)
        tile_type = m.background_tile;
    int x, y, timeout = 0;
    do
    {
        x = rnd(limit_x - 1);
        y = rnd(limit_y - 1);
        ++timeout;
    } while (read_map_tile_type(m, x, y) != tile_type && timeout <= 100);
    if (timeout <= 100)
    {
        c.x = x;
        c.y = y;
        return true;
    }
    return false;
}

void set_map_tile_type(map_struct& m, int x, int y, int tile_type)
{
    if (tile_type < 0 || tile_type >= MAX_NUMBER_OF_TILE_TYPES)
        error_message("set_map_tile_type: wrong tile_type: " + str(tile_type));
    if (!in_map(x, y))
        return;
    fld(m, x, y).tile_type = tile_type;
}

void set_map_portal(map_struct& m, int x, int y, const std::string& file, int tx, int ty)
{
    if (!in_map(x, y))
        return;
    auto& f = fld(m, x, y);
    f.portal_mapfile = file;
    f.portal_x = tx;
    f.portal_y = ty;
}

void set_map_message(map_struct& m, int x, int y, const std::string& l1, const std::string& l2, int tile)
{
    if (!in_map(x, y))
        return;
    auto& f = fld(m, x, y);
    f.message_line1 = l1;
    f.message_line2 = l2;
    f.message_tile = tile;
}

void set_map_event(map_struct& m, int x, int y, const std::string& ev, int xp)
{
    if (!in_map(x, y))
        return;
    fld(m, x, y).event = ev;
    fld(m, x, y).event_xp = xp;
}

void set_map_power_used(map_struct& m, int x, int y, const std::string& p)
{
    if (in_map(x, y))
        fld(m, x, y).power_used = p;
}

void add_map_power_used(map_struct& m, int x, int y, const std::string& p)
{
    if (in_map(x, y))
    {
        auto& f = fld(m, x, y);
        if (!compare_string_lists(f.power_used, p))
        {
            if (!f.power_used.empty())
                f.power_used += "|";
            f.power_used += p;
        }
    }
}

void set_map_visited(map_struct& m, int x, int y, int v)
{
    if (in_map(x, y))
        fld(m, x, y).visited = v;
}

void set_map_hidden(map_struct& m, int x, int y, int h)
{
    if (in_map(x, y))
        fld(m, x, y).hidden = h;
}

std::string read_map_message_line1(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).message_line1 : "";
}
std::string read_map_message_line2(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).message_line2 : "";
}
int read_map_message_tile(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).message_tile : 0;
}
std::string read_map_event(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).event : "";
}
int read_map_event_xp(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).event_xp : 0;
}
std::string read_map_power_used(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).power_used : "";
}
int read_map_if_power_used(const map_struct& m, int x, int y, const std::string& power)
{
    return in_map(x, y) ? compare_string_lists(fld(m, x, y).power_used, power) : 0;
}
int read_map_visited(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).visited : 0;
}
int read_map_hidden(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).hidden : 0;
}

int event_triggered(const std::string& ev)
{
    int rc = 0;
    for (auto& e : map_event)
        if (e.name == ev && e.triggered == 1)
            rc = 1;
    return rc;
}

int event_xp(const std::string& ev)
{
    int rc = 0;
    for (auto& e : map_event)
        if (e.name == ev)
            rc = e.xp;
    return rc;
}

int field_contains_transformation(int x, int y)
{
    for (auto& t : map_transformation)
        if (t.x == x && t.y == y)
            return 1;
    return 0;
}

int trigger_map_message(map_struct& m, int x, int y)
{
    if (!in_map(x, y))
        return 0;
    std::string l1 = read_map_message_line1(m, x, y), l2 = read_map_message_line2(m, x, y);
    if (!l1.empty() || !l2.empty())
    {
        message(l1, l2, rgb(255, 255, 255), read_map_message_tile(m, x, y), 0);
        return 1;
    }
    return 0;
}

std::string read_map_portal_mapfilename(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).portal_mapfile : "";
}
int read_map_portal_target_x(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).portal_x : 0;
}
int read_map_portal_target_y(const map_struct& m, int x, int y)
{
    return in_map(x, y) ? fld(m, x, y).portal_y : 0;
}

int field_is_blocked(const map_struct& m, int x, int y)
{
    int t = read_map_tile_type(m, x, y);
    return m.tileset.tile_type_db[t].solid == 1;
}

int field_is_blocked_for_monster(const map_struct& m, int x, int y, int monster_type)
{
    int rc = 0;
    int t = read_map_tile_type(m, x, y);
    const auto& tt = m.tileset.tile_type_db[t];
    const auto& mt = monster_type_db[monster_type];
    if (tt.solid == 1 && mt.flying == 0)
        rc = 1;
    if (tt.blocks_flight == 1 && mt.flying == 1)
        rc = 1;
    if (rc == 0 && field_contains_monster(x, y) == 1)
        rc = 1;
    return rc;
}

int field_blocks_flight(const map_struct& m, int x, int y)
{
    int t = read_map_tile_type(m, x, y);
    return m.tileset.tile_type_db[t].blocks_flight == 1;
}

int field_is_open_portal(const map_struct& m, int x, int y)
{
    return !read_map_portal_mapfilename(m, x, y).empty();
}

int field_contains_dungeon_entrance(int x, int y)
{
    int rc = 0;
    for (auto& d : dungeon_on_map)
        if (d.xpos == x && d.ypos == y)
            rc = 1;
    return rc;
}

void fill_map(map_struct& m, int tile_type)
{
    for (int y = 0; y < MAP_DIMENSION_Y; ++y)
        for (int x = 0; x < MAP_DIMENSION_X; ++x)
        {
            set_map_tile_type(m, x, y, tile_type);
            set_map_portal(m, x, y, "", 0, 0);
            set_map_message(m, x, y, "", "");
        }
}

int most_used_tile(const map_struct& m)
{
    std::vector<int> cnt(MAX_NUMBER_OF_TILE_TYPES, 0);
    for (int y = 0; y < MAP_DIMENSION_Y; ++y)
        for (int x = 0; x < MAP_DIMENSION_X; ++x)
            ++cnt[read_map_tile_type(m, x, y)];
    int rc = -1, best = 0;
    for (int i = 0; i < MAX_NUMBER_OF_TILE_TYPES; ++i)
        if (cnt[i] > best)
        {
            best = cnt[i];
            rc = i;
        }
    return rc;
}

int add_map_event(const std::string& ev, int triggered, int xp)
{
    for (auto& e : map_event)
        if (e.name == ev)
            return 0;
    map_event_struct e;
    e.name = ev;
    e.triggered = triggered;
    e.xp = xp;
    map_event.push_back(e);
    return 1;
}

void build_map_event_list(map_struct& m, int include_transformation_events)
{
    map_event.clear();
    for (int y = 0; y < MAP_DIMENSION_Y; ++y)
        for (int x = 0; x < MAP_DIMENSION_X; ++x)
        {
            std::string ev = read_map_event(m, x, y);
            if (!ev.empty())
                add_map_event(ev, 0, read_map_event_xp(m, x, y));
        }
    if (include_transformation_events)
        for (auto& t : map_transformation)
            if (!t.new_field.event.empty())
                add_map_event(t.new_field.event, 0);
}

int add_map_transformation(const map_transformation_struct& nt)
{
    for (auto& t : map_transformation)
        if (t.x == nt.x && t.y == nt.y && t.event == nt.event)
            return 0;
    map_transformation.push_back(nt);
    return 1;
}

void trigger_map_transformation(map_struct& m, const std::string& ev)
{
    // note: like the original, a transformation is not flagged as triggered here
    for (auto& t : map_transformation)
    {
        if (t.event == ev && t.triggered == 0)
        {
            set_map_tile_type(m, t.x, t.y, t.new_field.tile_type);
            set_map_portal(m, t.x, t.y, t.new_field.portal_mapfile, t.new_field.portal_x, t.new_field.portal_y);
            set_map_message(m, t.x, t.y, t.new_field.message_line1, t.new_field.message_line2,
                            t.new_field.message_tile);
            if (!t.new_field.event.empty())
            {
                set_map_event(m, t.x, t.y, t.new_field.event);
                add_map_event(t.new_field.event, 0);
            }
        }
    }
}

int trigger_map_event(map_struct& m, int x, int y)
{
    int rc = 0;
    std::string ev = read_map_event(m, x, y);
    if (!ev.empty())
        for (auto& e : map_event)
            if (e.name == ev && e.triggered != 1)
            {
                e.triggered = 1;
                rc = 1;
            }
    if (rc == 1)
    {
        int xp = event_xp(ev);
        if (xp > 0)
            experience(xp);
        trigger_map_transformation(m, ev);
    }
    return rc;
}

void reset_map(map_struct& m)
{
    m.name.clear();
    m.filename.clear();
    map_event.clear();
    map_transformation.clear();
    monster.clear();
    item_on_map.clear();
    dungeon_on_map.clear();
    reset_tileset(m.tileset);
    for (auto& f : m.mapfield)
        f = mapfield_struct{};
    m.tile_outside_map = m.default_tile = m.background_tile = 0;
    m.spawn_monster_level = m.spawn_monster_chance = 0;
    m.visited = 0;
}

// ================================================================ load / save
void load_map(map_struct& m, const std::string& path, bool load_tileset_)
{
    reset_map(m);
    XmlDoc d;
    if (!d.load(path))
        error_message("load_map(): could not open map file " + path + ": " + d.error);
    XmlNode* root = d.root.get();
    for (auto& a : root->attrs)
    {
        const std::string& k = a.first;
        const std::string& v = a.second;
        if (k == "name")
            m.name = v;
        else if (k == "tileset_definition_filename")
            m.tileset.definition_filename = file_part(v);
        else if (k == "tileset_image_filename")
            m.tileset.image_filename = file_part(v);
        else if (k == "default_tile")
            m.default_tile = (int)val(v);
        else if (k == "background_tile")
            m.background_tile = (int)val(v);
        else if (k == "tile_outside_map")
            m.tile_outside_map = (int)val(v);
        else if (k == "spawn_monster_level")
            m.spawn_monster_level = (int)val(v);
        else if (k == "spawn_monster_chance")
            m.spawn_monster_chance = (int)val(v);
        else if (k == "visited")
            m.visited = v == "yes";
        else if (k == "lighting")
            m.lighting = (int)val(v);
    }
    fill_map(m, m.default_tile);

    for (auto& n : root->children)
    {
        if (n->name == "field")
        {
            int x = -1, y = -1, portal_x = 0, portal_y = 0, event_trig = 0, msg_tile = 0, ev_xp = 0;
            int visited = 0, hidden = 0;
            std::string portal_file, l1, l2, ev, power_used;
            for (auto& a : n->attrs)
            {
                const std::string& k = a.first;
                const std::string& v = a.second;
                if (k == "x")
                    x = (int)val(v);
                else if (k == "y")
                    y = (int)val(v);
                else if (k == "portal_mapfile")
                    portal_file = v;
                else if (k == "portal_x")
                    portal_x = (int)val(v);
                else if (k == "portal_y")
                    portal_y = (int)val(v);
                else if (k == "message_line1")
                    l1 = v;
                else if (k == "message_line2")
                    l2 = v;
                else if (k == "event")
                    ev = v;
                else if (k == "event_triggered")
                    event_trig = (int)val(v);
                else if (k == "message_tile")
                    msg_tile = (int)val(v);
                else if (k == "event_xp")
                    ev_xp = (int)val(v);
                else if (k == "power_used")
                    power_used = v;
                else if (k == "visited")
                    visited = v == "yes";
                else if (k == "hidden")
                    hidden = v == "yes";
            }
            if (!in_map(x, y))
                continue;
            set_map_tile_type(m, x, y, (int)val(n->text));
            if (!portal_file.empty())
                set_map_portal(m, x, y, portal_file, portal_x, portal_y);
            set_map_message(m, x, y, l1, l2, msg_tile);
            set_map_power_used(m, x, y, power_used);
            set_map_hidden(m, x, y, hidden);
            set_map_visited(m, x, y, visited);
            if (!ev.empty())
            {
                set_map_event(m, x, y, ev, ev_xp);
                add_map_event(ev, event_trig, ev_xp);
            }
        }
        else if (n->name == "transformation")
        {
            map_transformation_struct t;
            t.x = t.y = -1;
            for (auto& a : n->attrs)
            {
                const std::string& k = a.first;
                const std::string& v = a.second;
                if (k == "x")
                    t.x = (int)val(v);
                else if (k == "y")
                    t.y = (int)val(v);
                else if (k == "portal_mapfile")
                    t.new_field.portal_mapfile = v;
                else if (k == "portal_x")
                    t.new_field.portal_x = (int)val(v);
                else if (k == "portal_y")
                    t.new_field.portal_y = (int)val(v);
                else if (k == "message_line1")
                    t.new_field.message_line1 = v;
                else if (k == "message_line2")
                    t.new_field.message_line2 = v;
                else if (k == "message_tile")
                    t.new_field.message_tile = (int)val(v);
                else if (k == "event")
                    t.event = v;
                else if (k == "new_event")
                    t.new_field.event = v;
                else if (k == "tile_type")
                    t.new_field.tile_type = (int)val(v);
                else if (k == "triggered")
                    t.triggered = (int)val(v);
            }
            add_map_transformation(t);
        }
        else if (n->name == "monster")
        {
            monster_struct nm;
            for (auto& a : n->attrs)
            {
                const std::string& k = a.first;
                const std::string& v = a.second;
                if (k == "type")
                    nm.type = (int)val(v);
                else if (k == "xpos")
                    nm.xpos = (int)val(v);
                else if (k == "ypos")
                    nm.ypos = (int)val(v);
                else if (k == "hitpoints")
                    nm.hitpoints = (int)val(v);
                else if (k == "alerted")
                    nm.alerted = v == "yes";
                else if (k == "effect")
                    nm.effect = v;
            }
            if (nm.type > 0)
                add_monster(m, nm);
        }
        else if (n->name == "item")
        {
            item_on_map_struct it;
            it.amount = 0;  // the original does not initialise the amount here
            for (auto& a : n->attrs)
            {
                if (a.first == "xpos")
                    it.xpos = (int)val(a.second);
                else if (a.first == "ypos")
                    it.ypos = (int)val(a.second);
                else if (a.first == "type")
                    it.type = (int)val(a.second);
            }
            if (it.type > 0)
                item_on_map.push_back(it);
        }
        else if (n->name == "dungeon")
        {
            dungeon_struct nd;
            nd.xpos = nd.ypos = -1;
            nd.entrance_to_level = 1;
            nd.difficulty_level = 1;
            nd.default_tile = 1;
            nd.background_tile = 4;
            nd.tile_outside_map = 1;
            for (auto& a : n->attrs)
            {
                const std::string& k = a.first;
                const std::string& v = a.second;
                if (k == "xpos")
                    nd.xpos = (int)val(v);
                else if (k == "ypos")
                    nd.ypos = (int)val(v);
                else if (k == "name")
                    nd.name = v;
                else if (k == "map_filename")
                    nd.map_filename = v;
                else if (k == "number_of_levels")
                    nd.number_of_levels = (int)val(v);
                else if (k == "dimension_x")
                    nd.dimension_x = (int)val(v);
                else if (k == "dimension_y")
                    nd.dimension_y = (int)val(v);
                else if (k == "entrance_to_level")
                    nd.entrance_to_level = (int)val(v);
                else if (k == "difficulty_level")
                    nd.difficulty_level = (int)val(v);
                else if (k == "lighting")
                    nd.lighting = (int)val(v);
                else if (k == "default_tile")
                    nd.default_tile = (int)val(v);
                else if (k == "background_tile")
                    nd.background_tile = (int)val(v);
                else if (k == "tile_outside_map")
                    nd.tile_outside_map = (int)val(v);
                else if (k == "main_treasure")
                    nd.main_treasure = (int)val(v);
                else if (k == "bottom_portal")
                    nd.bottom_portal = v;
                else if (k == "bottom_portal_target_x")
                    nd.bottom_portal_target_x = (int)val(v);
                else if (k == "bottom_portal_target_y")
                    nd.bottom_portal_target_y = (int)val(v);
                else if (k == "bottom_portal_tile")
                    nd.bottom_portal_tile = (int)val(v);
                else if (k == "traps_per_level")
                    nd.traps_per_level = (int)val(v);
            }
            if (!nd.name.empty())
                dungeon_on_map.push_back(nd);
        }
    }

    if (load_tileset_)
    {
        if (m.tileset.definition_filename.empty())
            error_message("load_map(): no tileset definition filename in " + path);
        if (m.tileset.image_filename.empty())
            error_message("load_map(): no tileset image filename in " + path);
        load_tileset(m.tileset);
    }
    m.filename = file_part(path);
}

void save_map(map_struct& m, const std::string& path)
{
    m.default_tile = most_used_tile(m);
    XmlDoc d;
    d.root = std::make_unique<XmlNode>();
    XmlNode* mn = d.root.get();
    mn->name = "map";
    mn->set("Lost_Labyrinth_version", version_number);
    mn->set("name", m.name);
    mn->set("filename", file_part(path));
    mn->set("tileset_definition_filename", m.tileset.definition_filename);
    mn->set("tileset_image_filename", m.tileset.image_filename);
    mn->set("tile_outside_map", str(m.tile_outside_map));
    mn->set("default_tile", str(m.default_tile));
    mn->set("background_tile", str(m.background_tile));
    mn->set("spawn_monster_level", str(m.spawn_monster_level));
    mn->set("spawn_monster_chance", str(m.spawn_monster_chance));
    mn->set("lighting", str(m.lighting));
    if (m.visited)
        mn->set("visited", "yes");
    for (int y = 0; y < MAP_DIMENSION_Y; ++y)
        for (int x = 0; x < MAP_DIMENSION_X; ++x)
        {
            int tile = read_map_tile_type(m, x, y);
            std::string pf = read_map_portal_mapfilename(m, x, y);
            std::string l1 = read_map_message_line1(m, x, y), l2 = read_map_message_line2(m, x, y);
            std::string ev = read_map_event(m, x, y), pu = read_map_power_used(m, x, y);
            int visited = read_map_visited(m, x, y), hidden = read_map_hidden(m, x, y);
            if (tile != m.default_tile || !pf.empty() || !ev.empty() || !l1.empty() || !l2.empty() || visited == 1 ||
                hidden == 1)
            {
                XmlNode* n = mn->add("field", str(tile));
                n->set("x", str(x));
                n->set("y", str(y));
                if (!pf.empty())
                {
                    n->set("portal_mapfile", pf);
                    n->set("portal_x", str(read_map_portal_target_x(m, x, y)));
                    n->set("portal_y", str(read_map_portal_target_y(m, x, y)));
                }
                if (!l1.empty() || !l2.empty())
                {
                    n->set("message_line1", l1);
                    n->set("message_line2", l2);
                    n->set("message_tile", str(read_map_message_tile(m, x, y)));
                }
                if (!pu.empty())
                    n->set("power_used", pu);
                if (visited == 1)
                    n->set("visited", "yes");
                if (hidden == 1)
                    n->set("hidden", "yes");
                if (!ev.empty())
                {
                    n->set("event", ev);
                    if (event_triggered(ev))
                        n->set("event_triggered", "1");
                    int xp = read_map_event_xp(m, x, y);
                    if (xp > 0)
                        n->set("event_xp", str(xp));
                }
            }
        }
    for (auto& t : map_transformation)
    {
        XmlNode* n = mn->add("transformation");
        n->set("event", t.event);
        n->set("x", str(t.x));
        n->set("y", str(t.y));
        n->set("tile_type", str(t.new_field.tile_type));
        n->set("portal_mapfile", t.new_field.portal_mapfile);
        n->set("portal_x", str(t.new_field.portal_x));
        n->set("portal_y", str(t.new_field.portal_y));
        n->set("message_line1", t.new_field.message_line1);
        n->set("message_line2", t.new_field.message_line2);
        n->set("message_tile", str(t.new_field.message_tile));
        n->set("new_event", t.new_field.event);
        n->set("triggered", str(t.triggered));
    }
    for (auto& mo : monster)
    {
        XmlNode* n = mn->add("monster");
        n->set("type", str(mo.type));
        n->set("xpos", str(mo.xpos));
        n->set("ypos", str(mo.ypos));
        n->set("hitpoints", str(mo.hitpoints));
        if (mo.alerted)
            n->set("alerted", "yes");
        if (!mo.effect.empty())
            n->set("effect", mo.effect);
    }
    for (auto& it : item_on_map)
    {
        XmlNode* n = mn->add("item");
        n->set("xpos", str(it.xpos));
        n->set("ypos", str(it.ypos));
        n->set("type", str(it.type));
    }
    for (auto& dg : dungeon_on_map)
    {
        XmlNode* n = mn->add("dungeon");
        n->set("xpos", str(dg.xpos));
        n->set("ypos", str(dg.ypos));
        n->set("name", dg.name);
        n->set("map_filename", dg.map_filename);
        n->set("number_of_levels", str(dg.number_of_levels));
        n->set("dimension_x", str(dg.dimension_x));
        n->set("dimension_y", str(dg.dimension_y));
        n->set("entrance_to_level", str(dg.entrance_to_level));
        n->set("difficulty_level", str(dg.difficulty_level));
        n->set("lighting", str(dg.lighting));
        n->set("default_tile", str(dg.default_tile));
        n->set("background_tile", str(dg.background_tile));
        n->set("tile_outside_map", str(dg.tile_outside_map));
        n->set("main_treasure", str(dg.main_treasure));
        n->set("bottom_portal", dg.bottom_portal);
        n->set("bottom_portal_target_x", str(dg.bottom_portal_target_x));
        n->set("bottom_portal_target_y", str(dg.bottom_portal_target_y));
        n->set("bottom_portal_tile", str(dg.bottom_portal_tile));
        n->set("traps_per_level", str(dg.traps_per_level));
    }
    if (!d.save(path))
        error_message("save_map(): could not save " + path);
}

// ================================================================ dungeon generation
static void dig_tunnel(map_struct& m, const dungeon_struct& d, dungeon_tunnel_struct& tn)
{
    int x = tn.start_xpos, y = tn.start_ypos;
    bool stop = false;
    if (read_map_tile_type(m, x, y) == m.background_tile)
        stop = true;
    else
        set_map_tile_type(m, x, y, m.background_tile);
    while (!stop)
    {
        int direction = rnd(3), count = 0;
        bool found = false;
        while (!stop && !found)
        {
            switch (direction)
            {
            case 0:
                if (y > 1 && read_map_tile_type(m, x, y - 2) == m.default_tile)
                {
                    set_map_tile_type(m, x, y - 1, m.background_tile);
                    set_map_tile_type(m, x, y - 2, m.background_tile);
                    found = true;
                    y -= 2;
                    tn.length += 2;
                }
                break;
            case 1:
                if (x < d.dimension_x - 2 && read_map_tile_type(m, x + 2, y) == m.default_tile)
                {
                    set_map_tile_type(m, x + 1, y, m.background_tile);
                    set_map_tile_type(m, x + 2, y, m.background_tile);
                    found = true;
                    x += 2;
                    tn.length += 2;
                }
                break;
            case 2:
                if (y < d.dimension_y - 2 && read_map_tile_type(m, x, y + 2) == m.default_tile)
                {
                    set_map_tile_type(m, x, y + 1, m.background_tile);
                    set_map_tile_type(m, x, y + 2, m.background_tile);
                    found = true;
                    y += 2;
                    tn.length += 2;
                }
                break;
            case 3:
                if (x > 1 && read_map_tile_type(m, x - 2, y) == m.default_tile)
                {
                    set_map_tile_type(m, x - 1, y, m.background_tile);
                    set_map_tile_type(m, x - 2, y, m.background_tile);
                    found = true;
                    x -= 2;
                    tn.length += 2;
                }
                break;
            }
            if (!found)
            {
                if (++direction > 3)
                    direction = 0;
                if (++count > 3)
                    stop = true;
            }
        }
    }
    tn.end_xpos = x;
    tn.end_ypos = y;
}

void create_dungeon_level(map_struct& m, const dungeon_struct& d)
{
    std::string new_map_filename = d.map_filename + "_" + str(d.entrance_to_level) + ".xml";
    reset_map(current_map);
    fill_map(current_map, d.default_tile);
    m.name = d.name + " " + message_list[MESSAGE_LEVEL] + " " + str(d.entrance_to_level);
    m.filename = new_map_filename;
    m.tileset.definition_filename = "tiles_standard.xml";
    m.tileset.image_filename = "tiles_standard.png";
    m.default_tile = d.default_tile;
    m.background_tile = d.background_tile;
    m.tile_outside_map = d.tile_outside_map;
    m.lighting = d.lighting;
    load_tileset(m.tileset);

    dungeon_tunnel_struct tn;
    tn.start_xpos = tn.start_ypos = 1;
    tn.length = 0;
    dig_tunnel(m, d, tn);
    set_map_portal(m, tn.start_xpos, tn.start_ypos, current_character.map_filename, current_character.xpos,
                   current_character.ypos);
    set_map_tile_type(m, tn.start_xpos, tn.start_ypos, 3);

    if (d.entrance_to_level < d.number_of_levels)
    {
        set_map_tile_type(m, tn.end_xpos, tn.end_ypos, 2);
        dungeon_struct nd = d;
        nd.xpos = tn.end_xpos;
        nd.ypos = tn.end_ypos;
        nd.entrance_to_level = d.entrance_to_level + 1;
        dungeon_on_map.push_back(nd);
    }
    else
    {
        item_on_map_struct it;
        it.xpos = tn.end_xpos;
        it.ypos = tn.end_ypos;
        it.type = d.main_treasure > 0 ? d.main_treasure : random_item_type(d.difficulty_level * 1000, "", 1);
        it.amount = 1;
        it.fuel = item_db[it.type].fuel;
        if (item_db[it.type].artefact == 1)
            it.magic_bonus = magic_bonus();
        item_on_map.push_back(it);
    }

    for (int y = 1; y <= d.dimension_y; y += 2)
        for (int x = 1; x <= d.dimension_x; x += 2)
        {
            if (read_map_tile_type(m, x, y) != m.default_tile)
                continue;
            int direction = rnd(3), count = 0;
            bool connected = false;
            while (count < 4 && !connected)
            {
                switch (direction)
                {
                case 0:
                    if (y > 1 && read_map_tile_type(m, x, y - 2) == m.background_tile)
                    {
                        set_map_tile_type(m, x, y - 1, m.background_tile);
                        connected = true;
                    }
                    break;
                case 1:
                    if (x < d.dimension_x - 2 && read_map_tile_type(m, x + 2, y) == m.background_tile)
                    {
                        set_map_tile_type(m, x + 1, y, m.background_tile);
                        connected = true;
                    }
                    break;
                case 2:
                    if (y < d.dimension_y - 2 && read_map_tile_type(m, x, y + 2) == m.background_tile)
                    {
                        set_map_tile_type(m, x, y + 1, m.background_tile);
                        connected = true;
                    }
                    break;
                case 3:
                    if (x > 1 && read_map_tile_type(m, x - 2, y) == m.background_tile)
                    {
                        set_map_tile_type(m, x - 1, y, m.background_tile);
                        connected = true;
                    }
                    break;
                }
                if (!connected)
                {
                    if (++direction > 3)
                        direction = 0;
                    ++count;
                }
                else
                {
                    tn.start_xpos = x;
                    tn.start_ypos = y;
                    tn.length = 0;
                    dig_tunnel(m, d, tn);
                    int rarity = d.entrance_to_level + d.difficulty_level * 10;
                    switch (rnd(3))
                    {
                    case 0: set_map_tile_type(m, tn.end_xpos, tn.end_ypos, 9); break;
                    case 1: add_random_item(tn.end_xpos, tn.end_ypos, rarity); break;
                    case 2:
                        set_map_tile_type(m, tn.end_xpos, tn.end_ypos, 6);
                        add_random_item(tn.end_xpos, tn.end_ypos, rarity);
                        add_random_item(tn.end_xpos, tn.end_ypos, rarity);
                        add_random_item(tn.end_xpos, tn.end_ypos, rarity);
                        break;
                    default: break;
                    }
                    set_map_tile_type(m, tn.end_xpos, tn.end_ypos, 9);
                }
            }
        }

    coordinates_struct c;
    if (d.traps_per_level > 0)
    {
        std::vector<int> traps;
        for (int i = 0; i < MAX_NUMBER_OF_TILE_TYPES; ++i)
            if (compare_string_lists(m.tileset.tile_type_db[i].class_, "trap"))
                traps.push_back(i);
        if (!traps.empty())
            for (int i = 1; i <= d.traps_per_level; ++i)
            {
                int t = traps[rnd((int)traps.size() - 1)];
                if (random_field(m, c, -1, d.dimension_x, d.dimension_y))
                {
                    set_map_tile_type(m, c.x, c.y, t);
                    set_map_hidden(m, c.x, c.y, 1);
                }
            }
    }
    for (int i = 0; i <= (d.dimension_x * d.dimension_y) / (5 * 5); ++i)
    {
        place_random_monster(m, d.entrance_to_level + d.difficulty_level - 1);
        if (random_field(m, c, -1, d.dimension_x, d.dimension_y))
        {
            set_map_tile_type(m, c.x, c.y, TILE_STANDARD_GOLD);
            set_map_hidden(m, c.x, c.y, 1);
        }
    }
    if (random_field(m, c, -1, d.dimension_x, d.dimension_y))
        set_map_tile_type(m, c.x, c.y, TILE_STANDARD_SANCTUM);
    if (random_field(m, c, -1, d.dimension_x, d.dimension_y))
        set_map_tile_type(m, c.x, c.y, TILE_STANDARD_NEXUS);
    if (d.entrance_to_level >= d.number_of_levels && !d.bottom_portal.empty())
        if (random_field(m, c, d.default_tile, d.dimension_x, d.dimension_y))
        {
            set_map_tile_type(m, c.x, c.y, d.bottom_portal_tile);
            set_map_portal(m, c.x, c.y, d.bottom_portal, d.bottom_portal_target_x, d.bottom_portal_target_y);
        }
    save_map(m, save_path(new_map_filename));
}

// ================================================================ portals / fields
void enter_portal()
{
    fade_map_screen_out(current_map);
    save_map(current_map, save_path(file_part(current_map.filename)));
    std::string nf = read_map_portal_mapfilename(current_map, current_character.xpos, current_character.ypos);
    int tx = read_map_portal_target_x(current_map, current_character.xpos, current_character.ypos);
    int ty = read_map_portal_target_y(current_map, current_character.xpos, current_character.ypos);
    if (pb::fileSize(save_path(file_part(nf))) > 0)
        load_map(current_map, save_path(file_part(nf)));
    else
        load_map(current_map, res_path("maps/" + file_part(nf)));
    current_character.xpos = tx;
    current_character.ypos = ty;
    current_character.map_filename = file_part(nf);
    if (current_map.visited != 1)
        current_map.visited = 1;
    fade_map_screen_in(current_map);
}

int enter_field()
{
    int rc = 0;
    int xpos = current_character.xpos, ypos = current_character.ypos;
    int tile_type = read_map_tile_type(current_map, xpos, ypos);
    const auto& tt = current_map.tileset.tile_type_db[tile_type];
    std::string message_line1 = tt.message, message_line2;
    std::string animation_enter = tt.animation_enter, sound = tt.sound;
    int remove_after_entering = tt.remove_after_entering;
    int effect_triggered = 0, negative_amount = 0, amount = 0;
    for (int i = 0; i < MAX_NUMBER_OF_ATTRIBUTES_MODIFIED_BY_TILE; ++i)
    {
        const auto& ma = tt.modify_attribute[i];
        int ability = ma.attribute;
        amount = ma.amount;
        if (read_map_hidden(current_map, xpos, ypos) == 0 || ma.trigger_hidden == 1)
        {
            if (ma.turn_ends == 1)
                rc = 1;
            if (amount < 0)
                negative_amount = 1;
            if (ability > 0 && amount != 0)
            {
                effect_triggered = 1;
                if (ma.amount_randomized == 1 && amount != 0)
                    amount = rnd(std::abs(amount) - 1) + 1;
                if (negative_amount == 1)
                    amount = amount * (-1);
                if (ma.amount_reduced_by_ability > 0)
                {
                    int cur = current_character.ability[ma.amount_reduced_by_ability].attribute_current_value;
                    if (negative_amount)
                        amount = imin(0, amount + cur);
                    else
                        amount = imax(0, amount - cur);
                }
                amount = modify_attribute(ability, amount);
                message_line2 = ability_db[ability].name_display;
                if (negative_amount)
                    message_line2 += " - " + str(std::abs(amount));
                else
                    message_line2 += " + " + str(amount);
            }
            if (ability > 0 && ma.replenish == 1)
                if (restore_attribute(ability) == 1)
                    effect_triggered = 1;
        }
    }
    if (!sound.empty() && effect_triggered == 1)
        play_sound(sound);
    if (!animation_enter.empty() && effect_triggered == 1)
        animation(animation_enter, 6, 6, amount);
    if (!message_line1.empty() && effect_triggered == 1)
        message(tt.message, message_line2, 0xffffff, tile_type, 0);
    if (remove_after_entering == 1 && effect_triggered == 1)
        set_map_tile_type(current_map, xpos, ypos, current_map.background_tile);

    message_line2.clear();
    int item_tile = 0, item_number = 0;
    for (auto& it : item_on_map)
        if (it.xpos == xpos && it.ypos == ypos)
        {
            ++item_number;
            if (!message_line2.empty())
                message_line2 += ", ";
            message_line2 += item_db[it.type].name;
            item_tile = item_db[it.type].tile;
        }
    if (item_number > 0)
    {
        message_line1 = (item_number == 1 ? message_list[MESSAGE_AN_ITEM_LIES_ON_THE_GROUND]
                                          : message_list[MESSAGE_SOME_ITEMS_LIE_ON_THE_GROUND]) +
                        ":";
        message(message_line1, message_line2, 0xffffff, item_tile, 1);
    }
    return rc;
}

void enter_dungeon()
{
    int tile_type = read_map_tile_type(current_map, current_character.xpos, current_character.ypos);
    dungeon_struct* dg = nullptr;
    for (auto& d : dungeon_on_map)
        if (d.xpos == current_character.xpos && d.ypos == current_character.ypos)
            dg = &d;
    if (!dg)
        return;
    dungeon_struct dcopy = *dg;  // the list is reset when the new level is loaded
    std::string text = replace_string(message_list[MESSAGE_ENTER_DUNGEON], "[dungeon_name]", dcopy.name);
    text = replace_string(text, "[dungeon_level]", str(dcopy.entrance_to_level));
    message(text, "", 0xffffff, tile_type, 0);
    fade_map_screen_out(current_map);
    save_map(current_map, save_path(file_part(current_map.filename)));
    std::string nf = dcopy.map_filename + "_" + str(dcopy.entrance_to_level) + ".xml";
    if (pb::fileSize(save_path(file_part(nf))) > 0)
        load_map(current_map, save_path(file_part(nf)));
    else
    {
        if (dcopy.entrance_to_level > 1)
            experience();
        create_dungeon_level(current_map, dcopy);
    }
    current_character.xpos = 1;
    current_character.ypos = 1;
    current_character.map_filename = nf;
    fade_map_screen_in(current_map);
}

int current_light_radius()
{
    int rc = current_map.lighting;
    int blind = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].lightradius > 0)
        {
            rc += attribute_current_val(i);
            if (current_character.ability[i].adjustment_percentage == 0)
                blind = 1;
        }
    if (blind == 1)
        rc = 0;
    return imax(0, imin(8, rc));
}

int detect_hidden_fields()
{
    int rc = 0;
    for (int x = current_character.xpos - 1; x <= current_character.xpos + 1; ++x)
        for (int y = current_character.ypos - 1; y <= current_character.ypos + 1; ++y)
        {
            if (read_map_visited(current_map, x, y) != 0)
                continue;
            set_map_visited(current_map, x, y);
            int tile_type = read_map_tile_type(current_map, x, y);
            if (read_map_hidden(current_map, x, y) == 1)
            {
                const auto& tt = current_map.tileset.tile_type_db[tile_type];
                if (rnd(99) < perception_val(tt.class_))
                {
                    set_map_hidden(current_map, x, y, 0);
                    rc = 1;
                    if (!tt.detection_message.empty())
                        message(tt.detection_message, "", 0xffffff, tile_type, 0);
                    if (!tt.detection_sound.empty())
                        play_sound(tt.detection_sound);
                }
            }
        }
    return rc;
}

}  // namespace ll
