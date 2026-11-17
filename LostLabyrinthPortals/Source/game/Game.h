// Lost Labyrinth: Portals - shared globals and function declarations
#pragma once

#include <functional>
#include <list>
#include <string>
#include <vector>

#include "game/Types.h"

namespace ll
{

using pb::imax;
using pb::imin;
using pb::rgb;
using pb::rnd;

inline pb::Platform& P()
{
    return pb::Platform::get();
}

// ---------------------------------------------------------------- global state (see constants.pb)
extern const char* version_number;
extern std::string program_title;
extern char_struct current_character;
extern map_struct current_map;
extern preferences_struct preferences;
extern message_struct message_;
extern std::list<map_event_struct> map_event;
extern std::list<map_transformation_struct> map_transformation;
extern std::vector<ability_db_struct> ability_db;
extern int program_ends;
extern std::list<monster_struct> monster;
extern std::vector<monster_type_db_struct> monster_type_db;
extern std::vector<key_binding_struct> key_binding;
extern std::vector<power_db_struct> power_db;
extern std::list<item_in_inventory_struct> inventory;
extern std::vector<item_db_struct> item_db;
extern std::list<item_on_map_struct> item_on_map;
extern std::list<dungeon_struct> dungeon_on_map;
extern std::vector<std::string> message_list;
extern int silhouette;
extern int startLastPoint;
extern int item_identification_global;
extern int wWidth, wHeight;
extern int sheets[SPRITE_COUNT];
extern int tile_columns[SPRITE_COUNT];  // columns of 32x32 tiles in the sheet

// ---------------------------------------------------------------- paths
std::string res_path(const std::string& rel);   // read-only game data (Content/...)
std::string save_path(const std::string& rel);  // savegame dir (writable)

// ---------------------------------------------------------------- misc (misc.pb)
bool compare_string_lists(const std::string& a, const std::string& b, char sep = '|');
std::string add_string_list(std::string list, const std::string& add, char sep = '|');
std::string replace_string(std::string s, const std::string& what, const std::string& with, bool nocase = false);
void error_message(const std::string& msg);
std::string yes_no_attr(const pb::XmlNode& n, const char* a);

// ---------------------------------------------------------------- preferences / messages / sound
void reset_key_bindings();
void load_preferences();
void save_preferences();
void load_message_list();
void load_sound();
void play_sound(const std::string& name);

// ---------------------------------------------------------------- graphics (graphics.pb)
void load_sprites();
void draw_sheet_tile(int sheet, int tile, float x, float y, bool transparent = true, int alpha = 255);
void draw_map_tile(const map_struct& map, int tile, float x, float y, bool transparent = true);
void draw_frame_main_screen();
void draw_map_screen(map_struct& map, int x_offset = 0, int y_offset = 0, int fade = 0);
void draw_character(int y_offset = 0);
void current_character_change_frame();
void draw_right_panel();
void draw_main_screen_complete(map_struct& map, int x_offset = 0, int y_offset = 0, int fade = 0,
                               int char_y_offset = 0);
void draw_game_scene();  // map + frame + panel + character (no flip)
void message(const std::string& line1, const std::string& line2, uint32_t color = 0xffffff, int tile = -1,
             int tile_type = 0, int display_for = 2000);
void fade_map_screen_out(map_struct& map);
void fade_map_screen_in(map_struct& map);
void draw_standard_frame(const std::string& title, const std::string& subtitle = "");
std::string shorten_text(std::string text, int width, const std::string& append = "...");
int wrap_text(int x, int y, const std::string& text, int width, int height = 16, uint32_t color = 0xffffff);
void screenshot(const std::string& filename);
void blink_cursor(cursor_struct& c);
void reset_cursor(cursor_struct& c);
void cursor_select_field(cursor_struct& c, int range = 2);
void star_sprite_animation(int star_sprite = 0, int x = 6, int y = 6, int number_of_stars = 7, int mixed = 0,
                           int second_star_sprite = 0);
void star_circle_animation(int star_sprite = 0, int x = 6, int y = 6);
void missile_animation(int tile, int source_x, int source_y, int destination_x, int destination_y);
void display_tile_animation(int tile, int type = 0, int x = 6, int y = 6, int display_number = 0, int number = 0);
void display_tile_flood_animation(int tile);
void animation(const std::string& name, int x = 6, int y = 6, int animation_parameter = 0);
void draw_text_centered_number(int x, int y, const std::string& text);  // damage numbers on a tile
std::string ability_value_display(int id);
std::string ability_expanded_value_display(int id);

// ---------------------------------------------------------------- menu (menu.pb)
struct menu_struct
{
    int x = 0, y = 0, width = 0, entry_height = 0, padding = 2, spacing = 4;
    int selected_entry = 0;
    int number_of_entries = 0;
    uint32_t color = 0xffffff;
    int use_mouse = 0;
    int mouse_x = 0, mouse_y = 0, mouse_over_entry = -1;
    std::vector<std::string> menu_entry;
    int menu_open = 1;
    int border = 0, entry_border = 1;
    int font = 0;
    int entry_highlight = 0;
    uint32_t entry_highlight_color1 = 0, entry_highlight_color2 = 0;
};
void reset_menu(menu_struct& m);
void menu_reset_entries(menu_struct& m);
int add_menu_entry(menu_struct& m, const std::string& option);
void menu_auto_set(menu_struct& m);
void display_menu(menu_struct& m);
std::string main_menu();

// ---------------------------------------------------------------- map (map_handling.pb)
void reset_tile_type_db(tileset_struct& ts);
void load_tile_type_definition(tileset_struct& ts);
void reset_tileset(tileset_struct& ts);
void load_tileset(tileset_struct& ts);
int read_map_tile_type(const map_struct& m, int x, int y);
bool random_field(map_struct& m, coordinates_struct& c, int tile_type = -1, int limit_x = MAP_DIMENSION_X,
                  int limit_y = MAP_DIMENSION_Y);
void set_map_tile_type(map_struct& m, int x, int y, int tile_type);
void set_map_portal(map_struct& m, int x, int y, const std::string& file = "", int tx = 0, int ty = 0);
void set_map_message(map_struct& m, int x, int y, const std::string& l1, const std::string& l2, int tile = 0);
void set_map_event(map_struct& m, int x, int y, const std::string& ev, int xp = 0);
void set_map_power_used(map_struct& m, int x, int y, const std::string& p);
void add_map_power_used(map_struct& m, int x, int y, const std::string& p);
void set_map_visited(map_struct& m, int x, int y, int v = 1);
void set_map_hidden(map_struct& m, int x, int y, int h = 1);
std::string read_map_message_line1(const map_struct& m, int x, int y);
std::string read_map_message_line2(const map_struct& m, int x, int y);
int read_map_message_tile(const map_struct& m, int x, int y);
std::string read_map_event(const map_struct& m, int x, int y);
int read_map_event_xp(const map_struct& m, int x, int y);
std::string read_map_power_used(const map_struct& m, int x, int y);
int read_map_if_power_used(const map_struct& m, int x, int y, const std::string& power);
int read_map_visited(const map_struct& m, int x, int y);
int read_map_hidden(const map_struct& m, int x, int y);
int event_triggered(const std::string& ev);
int event_xp(const std::string& ev);
int field_contains_transformation(int x, int y);
int trigger_map_message(map_struct& m, int x, int y);
std::string read_map_portal_mapfilename(const map_struct& m, int x, int y);
int read_map_portal_target_x(const map_struct& m, int x, int y);
int read_map_portal_target_y(const map_struct& m, int x, int y);
int field_is_blocked(const map_struct& m, int x, int y);
int field_is_blocked_for_monster(const map_struct& m, int x, int y, int monster_type);
int field_blocks_flight(const map_struct& m, int x, int y);
int field_is_open_portal(const map_struct& m, int x, int y);
int field_contains_dungeon_entrance(int x, int y);
void fill_map(map_struct& m, int tile_type);
int most_used_tile(const map_struct& m);
int add_map_event(const std::string& ev, int triggered = 0, int xp = 0);
void build_map_event_list(map_struct& m, int include_transformation_events = 1);
int add_map_transformation(const map_transformation_struct& t);
void trigger_map_transformation(map_struct& m, const std::string& ev);
int trigger_map_event(map_struct& m, int x, int y);
void reset_map(map_struct& m);
void load_map(map_struct& m, const std::string& path, bool load_tileset_ = true);
void save_map(map_struct& m, const std::string& path);
void create_dungeon_level(map_struct& m, const dungeon_struct& d);
void enter_portal();
int enter_field();
void enter_dungeon();
int current_light_radius();
int detect_hidden_fields();
std::string file_part(const std::string& p);

// ---------------------------------------------------------------- character (character.pb)
void init_character(char_struct& c);
void reset_ability(ability_db_struct& a);
void ability_db_resolve_names();
void load_abilities_db();
void save_character(char_struct& c, const std::string& path);
void load_character(char_struct& c, const std::string& path);
int attribute_cap(int id);
int attribute_max_val(int id, int cap = 1, int power_adj = 1, int item_adj = 1);
int attribute_current_val(int id, int cap = 1, int power_adj = 1, int item_adj = 1);
std::string ability_bonus_text(int id);
int ability_capped(int id, int bonus = 0);
int ability_available(int id);
void character_info_screen();  // draws (caller flips)
int abilities_available_for_xp();
int experience_screen(int selected_entry, int number_of_entries, int offset, const std::string& title,
                      uint32_t selected_entry_color = 0x500000);
void experience(int experience_points = 1, int game_start = 0);
void delete_savegame();
int restore_attribute(int id);
void reduce_ability(int id, int value, int negative_value_allowed = 0);
int increase_attribute(int id, int value);
int modify_attribute(int id, int value);
int modify_buffer_attribute(int id, int value);
int character_died();
int random_damage_in_melee();
int defense_in_melee();
int character_damage_buffer(int damage);
void character_takes_damage(int value, const std::string& game_end_message = "");
int sneaking_val(int only_when_activated = 0);
int backstabbing_val(int only_when_activated = 0);
int perception_val(const std::string& tile_class);
void update_abilities_adjustment_other();
int movement_left();
int armor_protection();
int random_armor_protection();

// ---------------------------------------------------------------- monsters (monster.pb)
void reset_monster_type(monster_type_db_struct& m);
void load_monster_db();
int field_contains_monster(int x, int y);
int add_monster(map_struct& m, const monster_struct& nm);
void clear_selected_monster();
int kill_monster(int kill_selected = 0);
void character_attacks_monster(map_struct& m, int x, int y);
void monster_attacks_character(int x, int y);
int random_monster_type(int max_level = 99999);
int place_random_monster(map_struct& m, int max_level = 99999);
int spawn_random_monster(map_struct& m);
void move_all_monster();
void monster_turn_ends();
int alert_monster(int x, int y);
int monster_is_alerted(int x, int y);
int monster_adjust_damage(int monster_type, int damage, const std::string& damage_class,
                          const std::string& affects_only = "");

// ---------------------------------------------------------------- powers (powers.pb)
void reset_power(power_db_struct& p);
void load_power_db();
void powers_db_resolve_names();
int power_available(int power_id, int gain_attribute = 0, int gain_bonus = 0);
int use_power();
int power_strength(int id);
void use_power_effect(int power_id);

// ---------------------------------------------------------------- items (items.pb)
void load_item_db();
void item_db_resolve_names();
void scramble_item_db_tiles(const std::string& cls);
int number_of_equipped_items(const std::string& cls);
void update_equipment_boni();
void open_inventory();
void character_gets_item(int type, int amount = 1, int fuel = -1, int magic_bonus = -1);
int character_loses_items(int amount_lost = 1, int all = 0);
void character_gets_starting_equipment();
int pickup_items(int x, int y);
int number_of_item_types();
int magic_bonus();
int random_item_type(int rarity = 5, const std::string& cls = "", int artefact = 0);
void add_random_item(int x, int y, int rarity = 5, const std::string& cls = "");

// ---------------------------------------------------------------- game thread entry
void game_main();  // runs the whole game (blocking); throws pb::QuitException on exit

}  // namespace ll
