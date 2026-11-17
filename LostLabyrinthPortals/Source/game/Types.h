// Lost Labyrinth: Portals - data structures (port of constants.pb)
#pragma once

#include <array>
#include <list>
#include <string>
#include <vector>

#include "pb/Platform.h"
#include "pb/Xml.h"

namespace ll
{

// ---------------------------------------------------------------- constants
constexpr int MAP_DIMENSION_X = 50;
constexpr int MAP_DIMENSION_Y = 50;
constexpr int MAX_NUMBER_OF_TILE_TYPES = 500;
constexpr int MAX_NUMBER_OF_ATTRIBUTES_MODIFIED_BY_TILE = 3;
constexpr int MAX_NUMBER_OF_ABILITIES = 255;
constexpr int MAX_NUMBER_OF_ATTRIBUTE_BONI = 10;
constexpr int MAX_NUMBER_OF_ABILITY_CAPS = 3;
constexpr int MAX_NUMBER_OF_ABILITY_REQUIREMENTS = 3;
constexpr int MAX_NUMBER_OF_ABILITY_DISPLAYS = 5;
constexpr int MAX_NUMBER_OF_ABILITY_ADJUSTMENTS = 5;
constexpr int MAX_NUMBER_OF_ABILITY_STARTING_EQUIPMENT = 3;
constexpr int MAX_NUMBER_OF_POWERS = 255;
constexpr int MAX_NUMBER_OF_POWER_REQUIREMENTS = 3;
constexpr int MAX_NUMBER_OF_MONSTER_TYPES = 300;
constexpr int MAX_NUMBER_OF_MONSTER_AFFLICTIONS = 5;
constexpr int MAX_NUMBER_OF_ITEMS = 255;
constexpr int MAX_NUMBER_OF_ITEM_ABILITY_BONI = 5;
constexpr int MAX_NUMBER_OF_ITEM_UNEQUIP_CLASSES = 5;
constexpr int MAX_NUMBER_OF_MESSAGES = 55;
constexpr int SETTING_MAP_SCREEN_SCROLLING_STEP = 2;
constexpr int INVENTORY_ROWS = 6;
constexpr int INVENTORY_COLUMNS = 6;
constexpr int TILE_ITEM_EQUIPPED = 47;
constexpr int TILE_STANDARD_GOLD = 9;
constexpr int TILE_STANDARD_SANCTUM = 140;
constexpr int TILE_STANDARD_NEXUS = 141;
constexpr int EXPERIENCE_SCREEN_ABILITIES_LEFT_BORDER = 56;
constexpr int EXPERIENCE_SCREEN_ABILITIES_TOP_BORDER = 52;
constexpr int EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN = 11;

// message ids
enum Msg
{
    MESSAGE_NULL = 0,
    MESSAGE_CHARACTER_INFO,
    MESSAGE_PRESS_RETURN_TO_LEAVE,
    MESSAGE_NOT_AVAILABLE,
    MESSAGE_CAPPED,
    MESSAGE_CURRENT_VALUE,
    MESSAGE_CAPPED_AT,
    MESSAGE_REQUIRED,
    MESSAGE_NEW_POWER,
    MESSAGE_YOU_GAIN_SOME_EXPERIENCE,
    MESSAGE_CHOOSE_STARTING_ABILITIES,
    MESSAGE_STRENGTH,
    MESSAGE_DURATION,
    MESSAGE_INVENTORY,
    MESSAGE_ITEMS,
    MESSAGE_AMOUNT,
    MESSAGE_WEIGHT,
    MESSAGE_POUND,
    MESSAGE_DROP,
    MESSAGE_CANCEL,
    MESSAGE_EQUIPMENT_LIMIT_REACHED,
    MESSAGE_PICKUP_ITEMS,
    MESSAGE_NO_ITEMS_TO_PICKUP,
    MESSAGE_PICKUP_AN_ITEM,
    MESSAGE_PICKUP_SOME_ITEMS,
    MESSAGE_LEVEL,
    MESSAGE_AN_ITEM_LIES_ON_THE_GROUND,
    MESSAGE_SOME_ITEMS_LIE_ON_THE_GROUND,
    MESSAGE_ENTER_DUNGEON,
    MESSAGE_MENU_SPLASH_NEW_GAME,
    MESSAGE_MENU_SPLASH_CONTINUE_GAME,
    MESSAGE_MENU_SPLASH_EXIT,
    MESSAGE_ITEM_MENU_UNEQUIP,
    MESSAGE_ITEM_MENU_EQUIP,
    MESSAGE_ITEM_MENU_DROP_ALL,
    MESSAGE_USE_A_POWER,
    MESSAGE_SUCCESS_CHANCE,
    MESSAGE_COST,
    MESSAGE_RANGE,
    MESSAGE_RANGE_SELF,
    MESSAGE_RANGE_TOUCH,
    MESSAGE_RANGE_SIGHT,
    MESSAGE_MONSTER_NOTICES_YOU,
    MESSAGE_YOU_SNEAK_UNNOTICED_BY,
    MESSAGE_YOU_WERE_KILLED_BY,
    MESSAGE_SUCCESSFUL_BACKSTAB,
    MESSAGE_INSUFFICIENT_POWER_RESOURCE,
    MESSAGE_INSUFFICIENT_POWER_RESOURCE2,
    MESSAGE_COOLDOWN_TOO_HIGH,
    MESSAGE_COOLDOWN,
    MESSAGE_MONSTER_REGENERATES,
    MESSAGE_MONSTER_IS_IMMUNE_VS_POWER,
    MESSAGE_MONSTER_ALREADY_AFFECTED_BY_POWER,
    MESSAGE_MONSTER_HAS_RESISTED,
    MESSAGE_UNIDENTIFIED_ITEM
};

// sprite sheets (ids into pb::Platform sheets, filled by load_sprites())
enum SheetId
{
    SPRITE_Characters,
    SPRITE_Frame,
    SPRITE_ABILITIES,
    SPRITE_SPLASH,
    SPRITE_MONSTER,
    SPRITE_MOUSEPOINTER,
    SPRITE_BLOOD,
    SPRITE_POWERS,
    SPRITE_CURSOR,
    SPRITE_STARS,
    SPRITE_MISSILES,
    SPRITE_LIGHT1,  // .. SPRITE_LIGHT1 + 6
    SPRITE_LIGHT7 = SPRITE_LIGHT1 + 6,
    SPRITE_ITEMS,
    SPRITE_FX,
    SPRITE_SILHOUETTE,
    SPRITE_BACKGROUND,
    SPRITE_COUNT
};

// fx tiles
constexpr int SPRITE_FX_GREY_BOX = 0;
constexpr int SPRITE_FX_CLOUD_RED = 1;
constexpr int SPRITE_FX_CLOUD_GREEN = 2;
constexpr int SPRITE_FX_CLOUD_BLUE = 3;
constexpr int SPRITE_FX_BLOOD_SKULL = 4;
constexpr int SPRITE_FX_MAGIC_SHIELD = 5;
constexpr int SPRITE_FX_RAGE = 6;
constexpr int SPRITE_FX_FIRE = 7;
constexpr int SPRITE_FX_WIND = 8;
constexpr int SPRITE_FX_CROSS = 9;
// missile tiles
constexpr int SPRITE_MISSILE_RED_BALL = 0;
constexpr int SPRITE_MISSILE_BLUE_BALL = 1;
constexpr int SPRITE_MISSILE_GREEN_BALL = 2;
constexpr int SPRITE_MISSILE_PURPLE_BALL = 3;
constexpr int SPRITE_MISSILE_LIGHTNING = 4;
constexpr int SPRITE_MISSILE_BLUE_LIGHTNING = 5;

// actions / key bindings
enum Action
{
    ACTION_TEST,
    ACTION_EXIT,
    ACTION_ESCAPE,
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_RETURN,
    ACTION_END_TURN,
    MAX_NUMBER_OF_ACTIONS
};

struct key_binding_struct
{
    std::string name;
    int keycode = 0;
};

// ---------------------------------------------------------------- tiles / tilesets
struct modify_attribute_by_tile_struct
{
    int attribute = 0;
    std::string attribute_name;
    int amount = 0;
    int amount_randomized = 0;
    int amount_reduced_by_ability = 0;
    std::string amount_reduced_by_ability_name;
    int replenish = 0;
    int trigger_hidden = 0;
    int turn_ends = 0;
};

struct tile_type_db_struct
{
    int in_use = 0;
    std::string name;
    int solid = 0;
    int blocks_flight = 0;
    int transparent = 0;
    std::string message;
    int remove_after_entering = 0;
    std::string sound;
    std::string class_;
    int transforms_into = 0;
    int container = 0;
    int hidden = 0;
    std::string detection_message;
    std::string detection_sound;
    std::string animation_enter;
    modify_attribute_by_tile_struct modify_attribute[MAX_NUMBER_OF_ATTRIBUTES_MODIFIED_BY_TILE];
};

struct tileset_struct
{
    std::string name;
    std::string image_filename;
    int sheet = -1;  // platform sheet id (the "sprite")
    int rows = 0;
    int columns = 0;
    std::string definition_filename;
    std::vector<tile_type_db_struct> tile_type_db = std::vector<tile_type_db_struct>(MAX_NUMBER_OF_TILE_TYPES);
};

// ---------------------------------------------------------------- map
struct map_event_struct
{
    std::string name;
    int xp = 0;
    int triggered = 0;
};

struct mapfield_struct
{
    int tile_type = 0;
    std::string portal_mapfile;
    int portal_x = 0;
    int portal_y = 0;
    std::string message_line1;
    std::string message_line2;
    int message_tile = 0;
    std::string event;
    int event_xp = 0;
    std::string power_used;
    int hidden = 0;
    int visited = 0;
};

struct map_struct
{
    std::string name;
    std::string filename;
    tileset_struct tileset;
    std::vector<mapfield_struct> mapfield = std::vector<mapfield_struct>(MAP_DIMENSION_X * MAP_DIMENSION_Y);
    int tile_outside_map = 0;
    int default_tile = 0;
    int background_tile = 0;
    int spawn_monster_level = 0;
    int spawn_monster_chance = 0;
    int visited = 0;
    int lighting = 0;
};

struct map_transformation_struct
{
    std::string event;
    int x = 0;
    int y = 0;
    mapfield_struct new_field;
    int triggered = 0;
};

struct dungeon_tunnel_struct
{
    int start_xpos = 0, start_ypos = 0, end_xpos = 0, end_ypos = 0;
    int length = 0, minimum_length = 0, first_tunnel = 0;
};

struct dungeon_struct
{
    int xpos = 0, ypos = 0;
    std::string name;
    std::string map_filename;
    int number_of_levels = 0;
    int dimension_x = 0, dimension_y = 0;
    int entrance_to_level = 0;
    int difficulty_level = 0;
    int lighting = 0;
    int default_tile = 0;
    int background_tile = 0;
    int tile_outside_map = 0;
    int main_treasure = 0;
    std::string bottom_portal;
    int bottom_portal_target_x = 0, bottom_portal_target_y = 0;
    int bottom_portal_tile = 0;
    int traps_per_level = 0;
};

struct coordinates_struct
{
    int x = 0, y = 0;
};

// ---------------------------------------------------------------- misc
struct preferences_struct
{
    int fullscreen = 0;
    int message_delay = 0;
    int DDD_effects = 0;
    int fade_effect = 1;
    int inventory_smooth_scrolling = 0;
    std::string language = "English";
};

struct message_struct
{
    uint32_t color = 0xffffff;
    std::string line1, line2;
    int tile = -1;
    int tile_type = 0;  // 0=map tileset, 1=item, 2=power, 3=monster, 4=ability
    uint32_t start_time = 0;
    uint32_t display_for = 0;
};

struct cursor_struct
{
    int x = 6, y = 6;
    int frame = 0;
    int dir = 1;
    int esc = 0;
};

// ---------------------------------------------------------------- abilities
struct attribute_bonus_struct
{
    int attribute = 0;
    std::string name;
    int bonus = 0;
    int displayed = 0;
};

struct ability_cap_struct
{
    int ability = 0;
    std::string name;
    int bonus = 0;
    int multiplier = 1;
    int divisor = 1;
};

struct ability_requirement_struct
{
    int attribute = 0;
    std::string name;
    int value = 0;
};

struct ability_display_struct
{
    int type = 0;
    int start_value = 0;
    int end_value = 0;
    std::string message, message2;
    uint32_t color = 0xffffff;
    int expanded_value_display = 0;
};

struct ability_attribute_adjustment_struct
{
    int attribute = 0;
    std::string attribute_name;
    int value = 0;
    int percentage = 100;
    int percentage_attribute_multiplier = 0;
    int percentage_bonus = 0;
};

struct ability_starting_equipment_struct
{
    int item = 0;
    std::string item_name;
    int amount = 1;
};

struct ability_db_struct
{
    std::string name, name_display, description;
    int tile = 0;
    uint32_t color = 0xffffff;
    int attribute = 0, depletable = 0, percentage = 0, displayed_as_bonus = 0;
    int randomizer_ability = 0;
    std::string randomizer_ability_name;
    int main_screen = 0;
    int attribute_start_value = 0;
    int xp = 0;
    int game_ends = 0;
    int game_ends_val = 0;
    std::string game_ends_message;
    int not_shown_if_zero = 0;
    int minimum = -1;
    int change_per_turn_chance = 0;
    int change_per_turn = 0;
    int change_per_move = 0;
    int turn_ends_if_lower = -32000;
    int restore_at_end_of_turn = 0;
    int attack_melee = 0, critical_hit_melee = 0, defense_melee = 0, damage_in_melee = 0;
    int modify_with_each_kill = 0;
    int lightradius = 0;
    int modify_per_point_of_damage = 0;
    int hit_multiplier = 0;
    int hit_attribute = 0;
    std::string hit_attribute_name;
    int sneaking = 0, backstab = 0;
    std::string perception_tile_class;
    int modify_attribute = 0;
    std::string modify_attribute_name;
    int modify_multiplier = 1, modify_divisor = 1;
    std::string modify_animation, modify_sound;
    int armor = 0, movement = 0;
    int buffer_for_attribute = 0;
    std::string buffer_for_attribute_name;
    std::string buffer_animation;
    attribute_bonus_struct attribute_bonus[MAX_NUMBER_OF_ATTRIBUTE_BONI];
    ability_cap_struct ability_cap[MAX_NUMBER_OF_ABILITY_CAPS];
    ability_requirement_struct ability_requirement[MAX_NUMBER_OF_ABILITY_REQUIREMENTS];
    ability_display_struct display[MAX_NUMBER_OF_ABILITY_DISPLAYS];
    ability_attribute_adjustment_struct adjustment[MAX_NUMBER_OF_ABILITY_ADJUSTMENTS];
    ability_starting_equipment_struct starting_equipment[MAX_NUMBER_OF_ABILITY_STARTING_EQUIPMENT];
};

struct ability_struct
{
    int ability_value = 0;
    int attribute_max_value = 0;
    int attribute_current_value = 0;
    int power_adjustment = 0;
    int adjustment_duration = 0;
    std::string adjustment_name;
    int adjustment_power_id = 0;
    int adjustment_item_id = 0;
    int item_bonus = 0;
    int activated = 0;
    int adjustment_other = 0;
    int adjustment_percentage = 100;
};

// ---------------------------------------------------------------- monsters
struct monster_affliction_struct
{
    std::string class_;
    int chance = 0;
    int attribute = 0;
    std::string attribute_name;
    int value = 0;
    int randomized = 0;
    std::string animation, sound, message;
};

struct monster_type_db_struct
{
    std::string name, class_;
    int tile = 0, attack = 0, defense = 0, strength = 0, hitpoints = 0, armor = 0, level = 0;
    int speed = 0, flying = 0, regeneration = 0;
    std::string resistance_class, immune_class, vulnerable_class;
    monster_affliction_struct affliction[MAX_NUMBER_OF_MONSTER_AFFLICTIONS];
};

struct monster_struct
{
    int type = 0;
    int xpos = 0, ypos = 0;
    int hitpoints = 0;
    int alerted = 0;
    int x_offset = 0, y_offset = 0;
    int selected = 0;
    std::string effect;
};

// ---------------------------------------------------------------- powers
struct power_requirement_struct
{
    int attribute = 0;
    std::string attribute_name;
    int value = 0;
};

struct power_db_struct
{
    std::string name, description;
    int tile = 0;
    int effect = 0;
    std::string effect_name;
    int effect_strength = 0;
    int strength_bonus_attribute = 0;
    std::string strength_bonus_attribute_name;
    int strength_multiplier = 1, strength_divisor = 1;
    int strength_randomized = 0;
    int randomized_strength_bonus = 0;
    int randomized_strength_bonus_attribute = 0;
    std::string randomized_strength_bonus_attribute_name;
    int randomized_strength_multiplier = 1, randomized_strength_divisor = 1;
    int success_attribute = 0;
    std::string success_attribute_name;
    int success_attribute_multiplier = 1, success_attribute_divisor = 1;
    int success_bonus = 0;
    std::string success_message, failure_message, wrong_use_message, already_used_message;
    std::string animation;
    int end_turn = 0;
    std::string sound, failure_sound, wrong_use_sound;
    int cost = 0;
    int cost_attribute = 0;
    std::string cost_attribute_name;
    std::string class_;
    int range = 0;
    int modify_attribute = 0;
    std::string modify_attribute_name;
    int adjust_attribute = 0;
    std::string adjust_attribute_name;
    int duration = 0;
    int duration_bonus_attribute = 0;
    std::string duration_bonus_attribute_name;
    int duration_multiplier = 1, duration_divisor = 1;
    int activates_ability_id = 0;
    std::string activates_ability_name;
    int deplete_attribute_id = 0;
    std::string deplete_attribute_name;
    int cooldown = 0;
    std::string affects_only_monster_class;
    std::string monster_effect;
    std::string field_effect;
    power_requirement_struct power_requirement[MAX_NUMBER_OF_POWER_REQUIREMENTS];
};

struct char_power_struct
{
    int cooldown = 0;
};

struct power_selection_struct
{
    int selected_power = 0;
    int selected_power_id = 0;
    cursor_struct cursor;
};

// ---------------------------------------------------------------- items
struct item_bonus_struct
{
    int attribute = 0;
    std::string attribute_name;
    int value = 0;
    int magic_bonus_multiplier = 0;
    int randomized = 0;
    int randomized_bonus = 0;
    int duration = 0;
};

struct item_db_struct
{
    std::string name;
    int tile = 0, tile_equipped = 0;
    int weight = 0;
    int starting_equipment = 0;
    std::string class_, class_name, description;
    int can_be_equipped = 0;
    int price = 0;
    int fuel = 0;
    int fuel_consumed_per_turn = 0;
    int fuel_consumed_per_use = 0;
    int fuel_randomized = 0;
    std::string fuel_name, use_name, use_animation;
    int consumed_after_use = 0;
    int turn_ends_after_use = 0;
    std::string use_message;
    int artefact = 0;
    int equipment_limit = 0;
    int identified = 0;
    int identified_when_used = 0;
    int power_id = 0;
    std::string power_name;
    item_bonus_struct ability_bonus[MAX_NUMBER_OF_ITEM_ABILITY_BONI];
    std::string unequip_item_class[MAX_NUMBER_OF_ITEM_UNEQUIP_CLASSES];
};

struct item_in_inventory_struct
{
    int type = 0;
    int amount = 1;
    int fuel = 0;
    int equipped = 0;
    int selected = 0;
    int magic_bonus = 0;
};

struct item_on_map_struct
{
    int xpos = 0, ypos = 0;
    int type = 0;
    int amount = 1;
    int fuel = 0;
    int selected = 0;
    int selected_for_pickup = 0;
    int magic_bonus = 0;
};

struct item_identification_struct
{
    int identified = 0;
};

struct item_tile_list_struct
{
    int item = 0, tile = 0;
};

// ---------------------------------------------------------------- character
struct char_struct
{
    std::string name;
    int xpos = 0, ypos = 0;
    int facing = 0;
    int frame = 0;
    std::string map_filename;
    int inventory_cursor_x = 0, inventory_cursor_y = 0;
    std::vector<ability_struct> ability = std::vector<ability_struct>(MAX_NUMBER_OF_ABILITIES);
    std::vector<char_power_struct> power = std::vector<char_power_struct>(MAX_NUMBER_OF_POWERS);
    std::vector<item_identification_struct> item_identification =
        std::vector<item_identification_struct>(MAX_NUMBER_OF_ITEMS);
    std::string game_end_message;
};

struct star_sprites_struct
{
    int x = 0, y = 0, frame = 0, dir = 1, color = 0, delay = 0;
};

}  // namespace ll
