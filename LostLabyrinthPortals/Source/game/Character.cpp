// character.pb
#include "game/Game.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>

namespace ll
{

using pb::str;
using pb::val;
using pb::XmlDoc;
using pb::XmlNode;

// ================================================================ init
void init_character(char_struct& c)
{
    c.xpos = 2;
    c.ypos = 4;
    c.facing = 0;
    c.frame = 0;
    c.map_filename = "map_start.xml";
    c.inventory_cursor_x = c.inventory_cursor_y = 0;
    for (int i = 0; i < MAX_NUMBER_OF_ABILITIES; ++i)
    {
        ability_struct a;
        a.attribute_max_value = ability_db[i].attribute_start_value;
        a.attribute_current_value = ability_db[i].attribute_start_value;
        c.ability[i] = a;
    }
    for (auto& p : c.power)
        p.cooldown = 0;
    for (auto& it : c.item_identification)
        it.identified = item_identification_global;
    c.game_end_message.clear();
    inventory.clear();
    update_abilities_adjustment_other();
    scramble_item_db_tiles("Potion");
}

void reset_ability(ability_db_struct& a)
{
    a = ability_db_struct{};
}

// ================================================================ ability database
void ability_db_resolve_names()
{
    auto find_ability = [](const std::string& name)
    {
        int rc = 0;
        for (int k = 0; k < MAX_NUMBER_OF_ABILITIES; ++k)
            if (ability_db[k].name == name)
                rc = k;
        return rc;
    };
    for (int i = 0; i < MAX_NUMBER_OF_ABILITIES; ++i)
    {
        auto& a = ability_db[i];
        if (a.randomizer_ability == 0 && !a.randomizer_ability_name.empty())
            a.randomizer_ability = find_ability(a.randomizer_ability_name);
        if (a.modify_attribute == 0 && !a.modify_attribute_name.empty())
            a.modify_attribute = find_ability(a.modify_attribute_name);
        if (a.buffer_for_attribute == 0 && !a.buffer_for_attribute_name.empty())
            a.buffer_for_attribute = find_ability(a.buffer_for_attribute_name);
        if (a.hit_attribute == 0 && !a.hit_attribute_name.empty())
            a.hit_attribute = find_ability(a.hit_attribute_name);
        for (auto& b : a.attribute_bonus)
            if (b.attribute == 0 && !b.name.empty())
                b.attribute = find_ability(b.name);
        for (auto& b : a.ability_cap)
            if (b.ability == 0 && !b.name.empty())
                b.ability = find_ability(b.name);
        for (auto& b : a.ability_requirement)
            if (b.attribute == 0 && !b.name.empty())
                b.attribute = find_ability(b.name);
        for (auto& b : a.adjustment)
            if (b.attribute == 0 && !b.attribute_name.empty())
                b.attribute = find_ability(b.attribute_name);
        for (auto& b : a.starting_equipment)
            if (b.item == 0 && !b.item_name.empty())
                for (int k = 1; k < MAX_NUMBER_OF_ITEMS; ++k)
                    if (item_db[k].name == b.item_name)
                        b.item = k;
    }
}

void load_abilities_db()
{
    for (auto& a : ability_db)
        reset_ability(a);
    XmlDoc d;
    if (!d.load(res_path("data/abilities.xml")))
        error_message("load_abilities_db(): could not open data/abilities.xml: " + d.error);
    for (auto& n : d.root->children)
    {
        if (n->name != "ability")
            continue;
        int id = -1;
        ability_db_struct na;
        auto yes = [](const std::string& v) { return v == "yes" ? 1 : 0; };
        for (auto& at : n->attrs)
        {
            const std::string& k = at.first;
            const std::string& v = at.second;
            if (k == "id") id = (int)val(v);
            else if (k == "name") na.name = v;
            else if (k == "description") na.description = v;
            else if (k == "tile") na.tile = (int)val(v);
            else if (k == "attribute_start_value") na.attribute_start_value = (int)val(v);
            else if (k == "color") na.color = (uint32_t)val(v);
            else if (k == "depletable") na.depletable = yes(v);
            else if (k == "percentage") na.percentage = yes(v);
            else if (k == "main_screen") na.main_screen = yes(v);
            else if (k == "displayed_as_bonus") na.displayed_as_bonus = yes(v);
            else if (k == "randomizer_ability") na.randomizer_ability = (int)val(v);
            else if (k == "attribute") na.attribute = yes(v);
            else if (k == "xp") na.xp = yes(v);
            else if (k == "not_shown_if_zero") na.not_shown_if_zero = yes(v);
            else if (k == "minimum") na.minimum = (int)val(v);
            else if (k == "change_per_turn_chance") na.change_per_turn_chance = (int)val(v);
            else if (k == "change_per_turn") na.change_per_turn = (int)val(v);
            else if (k == "change_per_move") na.change_per_move = (int)val(v);
            else if (k == "turn_ends_if_lower") na.turn_ends_if_lower = (int)val(v);
            else if (k == "restore_at_end_of_turn") na.restore_at_end_of_turn = yes(v);
            else if (k == "game_ends") na.game_ends = v == "lower" ? 1 : (v == "higher" ? 2 : 0);
            else if (k == "game_ends_val") na.game_ends_val = (int)val(v);
            else if (k == "game_ends_message") na.game_ends_message = v;
            else if (k == "critical_hit_melee") na.critical_hit_melee = yes(v);
            else if (k == "attack_melee") na.attack_melee = yes(v);
            else if (k == "defense_melee") na.defense_melee = yes(v);
            else if (k == "modify_with_each_kill") na.modify_with_each_kill = (int)val(v);
            else if (k == "damage_in_melee") na.damage_in_melee = yes(v);
            else if (k == "randomizer_ability_name") na.randomizer_ability_name = v;
            else if (k == "lightradius") na.lightradius = yes(v);
            else if (k == "modify_per_point_of_damage") na.modify_per_point_of_damage = (int)val(v);
            else if (k == "sneaking") na.sneaking = yes(v);
            else if (k == "backstab") na.backstab = yes(v);
            else if (k == "perception_tile_class") na.perception_tile_class = v;
            else if (k == "modify_atribute" || k == "modify_attribute") na.modify_attribute = (int)val(v);
            else if (k == "modify_attribute_name") na.modify_attribute_name = v;
            else if (k == "modify_multiplier") na.modify_multiplier = (int)val(v);
            else if (k == "modify_divisor") na.modify_divisor = (int)val(v);
            else if (k == "modify_animation") na.modify_animation = v;
            else if (k == "modify_sound") na.modify_sound = v;
            else if (k == "armor") na.armor = yes(v);
            else if (k == "movement") na.movement = yes(v);
            else if (k == "buffer_for_attribute") na.buffer_for_attribute = (int)val(v);
            else if (k == "buffer_for_attribute_name") na.buffer_for_attribute_name = v;
            else if (k == "buffer_animation") na.buffer_animation = v;
            else if (k == "hit_multiplier") na.hit_multiplier = (int)val(v);
            else if (k == "hit_attribute") na.hit_attribute = (int)val(v);
            else if (k == "hit_attribute_name") na.hit_attribute_name = v;
        }
        if (id < 1 || id >= MAX_NUMBER_OF_ABILITIES)
            error_message("load_abilities_db(): wrong ability id " + str(id));
        if (!ability_db[id].name.empty())
            error_message("load_abilities_db(): ability id " + str(id) + " is used twice");
        na.name_display = na.name;
        ability_db[id] = na;
        auto& ab = ability_db[id];
        int bonus_idx = 0, cap_idx = 0, req_idx = 0, disp_idx = 0, adj_idx = 0, eq_idx = 0;
        for (auto& c : n->children)
        {
            bool lang_ok = c->attrOr("language") == preferences.language;
            if (c->name == "attribute_bonus")
            {
                if (bonus_idx >= MAX_NUMBER_OF_ATTRIBUTE_BONI)
                    error_message("too many attribute_bonus tags for ability " + ab.name);
                attribute_bonus_struct b;
                for (auto& at : c->attrs)
                {
                    if (at.first == "attribute") b.attribute = (int)val(at.second);
                    else if (at.first == "bonus") b.bonus = (int)val(at.second);
                    else if (at.first == "displayed") b.displayed = at.second == "yes";
                    else if (at.first == "name") b.name = at.second;
                }
                if (b.attribute > 0 || !b.name.empty())
                    ab.attribute_bonus[bonus_idx++] = b;
            }
            else if (c->name == "cap")
            {
                if (cap_idx >= MAX_NUMBER_OF_ABILITY_CAPS)
                    error_message("too many cap tags for ability " + ab.name);
                ability_cap_struct b;
                for (auto& at : c->attrs)
                {
                    if (at.first == "ability") b.ability = (int)val(at.second);
                    else if (at.first == "name") b.name = at.second;
                    else if (at.first == "bonus") b.bonus = (int)val(at.second);
                    else if (at.first == "divisor") b.divisor = (int)val(at.second);
                    else if (at.first == "multiplier") b.multiplier = (int)val(at.second);
                }
                if (b.ability > 0 || !b.name.empty())
                    ab.ability_cap[cap_idx++] = b;
            }
            else if (c->name == "requirement")
            {
                if (req_idx >= MAX_NUMBER_OF_ABILITY_REQUIREMENTS)
                    error_message("too many requirement tags for ability " + ab.name);
                ability_requirement_struct b;
                for (auto& at : c->attrs)
                {
                    if (at.first == "attribute") b.attribute = (int)val(at.second);
                    else if (at.first == "name") b.name = at.second;
                    else if (at.first == "value") b.value = (int)val(at.second);
                }
                if (b.attribute > 0 || !b.name.empty())
                    ab.ability_requirement[req_idx++] = b;
            }
            else if (c->name == "display")
            {
                ability_display_struct b;
                std::string lang;
                for (auto& at : c->attrs)
                {
                    const std::string& v = at.second;
                    if (at.first == "type")
                        b.type = v == "main screen status panel" ? 1 : (v == "character info" ? 2 : 0);
                    else if (at.first == "start_value") b.start_value = (int)val(v);
                    else if (at.first == "end_value") b.end_value = (int)val(v);
                    else if (at.first == "message") b.message = v;
                    else if (at.first == "message2") b.message2 = v;
                    else if (at.first == "color") b.color = (uint32_t)val(v);
                    else if (at.first == "language") lang = v;
                    else if (at.first == "expanded_value_display") b.expanded_value_display = v == "yes";
                }
                if (!b.message.empty() && !lang.empty() && lang == preferences.language)
                {
                    if (disp_idx >= MAX_NUMBER_OF_ABILITY_DISPLAYS)
                        error_message("too many display tags for ability " + ab.name);
                    ab.display[disp_idx++] = b;
                }
            }
            else if (c->name == "name")
            {
                if (lang_ok)
                    ab.name_display = c->text;
            }
            else if (c->name == "description")
            {
                if (lang_ok)
                    ab.description = c->text;
            }
            else if (c->name == "game_ends_message")
            {
                if (lang_ok)
                    ab.game_ends_message = c->text;
            }
            else if (c->name == "adjustment")
            {
                ability_attribute_adjustment_struct b;
                for (auto& at : c->attrs)
                {
                    const std::string& v = at.second;
                    if (at.first == "attribute") b.attribute = (int)val(v);
                    else if (at.first == "attribute_name") b.attribute_name = v;
                    else if (at.first == "value") b.value = (int)val(v);
                    else if (at.first == "percentage") b.percentage = (int)val(v);
                    else if (at.first == "percentage_attribute_multiplier") b.percentage_attribute_multiplier = (int)val(v);
                    else if (at.first == "percentage_bonus") b.percentage_bonus = (int)val(v);
                }
                if (adj_idx >= MAX_NUMBER_OF_ABILITY_ADJUSTMENTS)
                    error_message("too many adjustment tags for ability " + ab.name);
                ab.adjustment[adj_idx++] = b;
            }
            else if (c->name == "starting_equipment")
            {
                ability_starting_equipment_struct b;
                for (auto& at : c->attrs)
                {
                    if (at.first == "item") b.item = (int)val(at.second);
                    else if (at.first == "item_name") b.item_name = at.second;
                    else if (at.first == "amount") b.amount = (int)val(at.second);
                }
                if (b.item > 0 || !b.item_name.empty())
                {
                    if (eq_idx >= MAX_NUMBER_OF_ABILITY_STARTING_EQUIPMENT)
                        error_message("too many starting_equipment tags for ability " + ab.name);
                    ab.starting_equipment[eq_idx++] = b;
                }
            }
        }
    }
}

// ================================================================ save / load
void save_character(char_struct& c, const std::string& path)
{
    XmlDoc d;
    d.root = std::make_unique<XmlNode>();
    XmlNode* m = d.root.get();
    m->name = "character";
    m->set("Lost_Labyrinth_version", version_number);
    m->set("name", c.name);
    m->set("xpos", str(c.xpos));
    m->set("ypos", str(c.ypos));
    m->set("facing", str(c.facing));
    m->set("frame", str(c.frame));
    m->set("map_filename", file_part(c.map_filename));
    m->set("inventory_cursor_x", str(c.inventory_cursor_x));
    m->set("inventory_cursor_y", str(c.inventory_cursor_y));
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
    {
        if (ability_db[i].name.empty())
            continue;
        const auto& a = c.ability[i];
        XmlNode* n = m->add("ability");
        n->set("id", str(i));
        if (a.ability_value != 0) n->set("ability_value", str(a.ability_value));
        if (a.attribute_max_value != 0) n->set("attribute_max_value", str(a.attribute_max_value));
        if (a.attribute_current_value != 0) n->set("attribute_current_value", str(a.attribute_current_value));
        if (a.power_adjustment != 0) n->set("power_adjustment", str(a.power_adjustment));
        if (a.adjustment_duration != 0) n->set("adjustment_duration", str(a.adjustment_duration));
        if (!a.adjustment_name.empty()) n->set("adjustment_name", a.adjustment_name);
        if (a.adjustment_power_id != 0) n->set("adjustment_power_id", str(a.adjustment_power_id));
        if (a.adjustment_item_id != 0) n->set("adjustment_item_id", str(a.adjustment_item_id));
        if (a.item_bonus != 0) n->set("item_bonus", str(a.item_bonus));
        if (a.activated == 1) n->set("activated", "yes");
        if (a.adjustment_other != 0) n->set("adjustment_other", str(a.adjustment_other));
        if (a.adjustment_percentage != 100) n->set("adjustment_percentage", str(a.adjustment_percentage));
    }
    for (int i = 0; i < MAX_NUMBER_OF_POWERS; ++i)
        if (c.power[i].cooldown > 0)
        {
            XmlNode* n = m->add("power");
            n->set("id", str(i));
            n->set("cooldown", str(c.power[i].cooldown));
        }
    for (auto& it : inventory)
    {
        XmlNode* n = m->add("item");
        n->set("type", str(it.type));
        n->set("amount", str(it.amount));
        if (item_db[it.type].fuel > 0) n->set("fuel", str(it.fuel));
        if (it.equipped == 1) n->set("equipped", "yes");
        if (it.selected == 1) n->set("selected", "yes");
        if (it.magic_bonus > 0) n->set("magic_bonus", str(it.magic_bonus));
    }
    for (int i = 0; i < MAX_NUMBER_OF_ITEMS; ++i)
    {
        if (c.item_identification[i].identified == 1)
        {
            XmlNode* n = m->add("item_identification");
            n->set("id", str(i));
            n->set("identified", "yes");
        }
        if (compare_string_lists(item_db[i].class_, "potion"))
        {
            XmlNode* n = m->add("item_tile");
            n->set("id", str(i));
            n->set("tile", str(item_db[i].tile));
        }
    }
    if (!d.save(path))
        error_message("save_character: could not save " + path);
}

void load_character(char_struct& c, const std::string& path)
{
    init_character(c);
    inventory.clear();
    XmlDoc d;
    if (!d.load(path))
        error_message("load_character(): could not open " + path + ": " + d.error);
    XmlNode* m = d.root.get();
    for (auto& a : m->attrs)
    {
        const std::string& v = a.second;
        if (a.first == "name") c.name = v;
        else if (a.first == "xpos") c.xpos = (int)val(v);
        else if (a.first == "ypos") c.ypos = (int)val(v);
        else if (a.first == "facing") c.facing = (int)val(v);
        else if (a.first == "frame") c.frame = (int)val(v);
        else if (a.first == "map_filename") c.map_filename = file_part(v);
        else if (a.first == "inventory_cursor_x") c.inventory_cursor_x = (int)val(v);
        else if (a.first == "inventory_cursor_y") c.inventory_cursor_y = (int)val(v);
    }
    for (auto& n : m->children)
    {
        if (n->name == "ability")
        {
            int id = (int)val(n->attrOr("id", "0"));
            if (id < 1 || id > MAX_NUMBER_OF_ABILITIES - 1)
                continue;
            ability_struct a;
            for (auto& at : n->attrs)
            {
                const std::string& v = at.second;
                if (at.first == "ability_value") a.ability_value = (int)val(v);
                else if (at.first == "attribute_max_value") a.attribute_max_value = (int)val(v);
                else if (at.first == "attribute_current_value") a.attribute_current_value = (int)val(v);
                else if (at.first == "power_adjustment") a.power_adjustment = (int)val(v);
                else if (at.first == "adjustment_duration") a.adjustment_duration = (int)val(v);
                else if (at.first == "adjustment_name") a.adjustment_name = v;
                else if (at.first == "adjustment_power_id") a.adjustment_power_id = (int)val(v);
                else if (at.first == "adjustment_item_id") a.adjustment_item_id = (int)val(v);
                else if (at.first == "item_bonus") a.item_bonus = (int)val(v);
                else if (at.first == "activated") a.activated = v == "yes";
                else if (at.first == "adjustment_other") a.adjustment_other = (int)val(v);
                else if (at.first == "adjustment_percentage") a.adjustment_percentage = (int)val(v);
            }
            c.ability[id] = a;
        }
        else if (n->name == "power")
        {
            int id = (int)val(n->attrOr("id", "0"));
            if (id > 0 && id < MAX_NUMBER_OF_POWERS)
                c.power[id].cooldown = (int)val(n->attrOr("cooldown", "0"));
        }
        else if (n->name == "item")
        {
            item_in_inventory_struct it;
            it.amount = 0;
            it.type = (int)val(n->attrOr("type", "0"));
            it.amount = (int)val(n->attrOr("amount", "0"));
            it.equipped = n->attrOr("equipped") == "yes";
            it.selected = n->attrOr("selected") == "yes";
            it.fuel = (int)val(n->attrOr("fuel", "0"));
            it.magic_bonus = (int)val(n->attrOr("magic_bonus", "0"));
            if (it.type > 0)
                inventory.push_back(it);
        }
        else if (n->name == "item_identification")
        {
            int id = (int)val(n->attrOr("id", "0"));
            if (id > 0 && id < MAX_NUMBER_OF_ITEMS)
                c.item_identification[id].identified = n->attrOr("identified") == "yes";
        }
        else if (n->name == "item_tile")
        {
            int id = (int)val(n->attrOr("id", "0"));
            if (id > 0 && id < MAX_NUMBER_OF_ITEMS)
                item_db[id].tile = (int)val(n->attrOr("tile", "0"));
        }
    }
}

// ================================================================ attributes
int attribute_cap(int id)
{
    int rc = -1;
    for (int i = 0; i < MAX_NUMBER_OF_ABILITY_CAPS; ++i)
    {
        const auto& cp = ability_db[id].ability_cap[i];
        if (cp.ability > 0)
        {
            int amv = current_character.ability[cp.ability].attribute_max_value;
            int cap = amv;
            if (cp.multiplier > 1)
                cap = amv * cp.multiplier;
            if (cp.divisor > 1)
                cap = amv / cp.divisor;
            cap += cp.bonus;
            if (cap < rc || rc == -1)
                rc = cap;
        }
    }
    return rc;
}

int attribute_max_val(int id, int cap_, int power_adj, int item_adj)
{
    int rc = current_character.ability[id].attribute_max_value;
    if (cap_ == 1)
    {
        int cap = attribute_cap(id);
        if (cap != -1)
            rc = imin(rc, cap);
    }
    if (power_adj == 1)
        rc += current_character.ability[id].power_adjustment;
    if (item_adj == 1)
        rc += current_character.ability[id].item_bonus;
    rc += current_character.ability[id].adjustment_other;
    if (current_character.ability[id].adjustment_percentage != 100)
        rc = rc * (current_character.ability[id].adjustment_percentage / 100);  // integer division, as in the original
    return rc;
}

int attribute_current_val(int id, int cap_, int power_adj, int item_adj)
{
    const auto& ab = current_character.ability[id];
    const auto& db = ability_db[id];
    int rc = ab.attribute_current_value;
    // (the original also tested ability_db(i)\movement with an uninitialised i = 0, i.e. always false)
    if (cap_ == 1 && db.change_per_move == 0)
    {
        int cap = attribute_cap(id);
        if (cap != -1)
            rc = imin(rc, cap);
    }
    if (power_adj == 1 && db.change_per_move == 0)
        rc += ab.power_adjustment;
    if (item_adj == 1 && db.change_per_move == 0)
        rc += ab.item_bonus;
    if (db.depletable == 0 && db.change_per_move == 0)
    {
        rc += ab.adjustment_other;
        if (ab.adjustment_percentage != 100)
            rc = (rc * ab.adjustment_percentage) / 100;
    }
    return rc;
}

std::string ability_expanded_value_display(int id)
{
    std::string rc;
    int cur = attribute_current_val(id);
    if (ability_db[id].displayed_as_bonus)
        rc = current_character.ability[id].attribute_current_value < 0 ? "-" : "+";
    if (ability_db[id].randomizer_ability > 0)
    {
        int v1 = cur, v2 = attribute_current_val(ability_db[id].randomizer_ability);
        rc += str(v1) + " - " + str(v1 + v2);
    }
    else
    {
        rc += str(attribute_current_val(id));
        if (ability_db[id].depletable == 1)
            rc += " / " + str(attribute_max_val(id));
    }
    if (ability_db[id].percentage == 1)
        rc += "%";
    return rc;
}

std::string ability_value_display(int id)
{
    std::string rc = ability_expanded_value_display(id);
    int cur = 0;  // the original never assigns its local; kept for parity
    for (int i = 0; i < MAX_NUMBER_OF_ABILITY_DISPLAYS; ++i)
        if (!ability_db[id].display[i].message.empty())
            if (cur >= ability_db[id].display[i].start_value && cur <= ability_db[id].display[i].end_value)
                rc = replace_string(ability_db[id].display[i].message, "[value]", str(cur), true);
    return rc;
}

std::string ability_bonus_text(int id)
{
    std::string text;
    for (int i = 0; i < MAX_NUMBER_OF_ATTRIBUTE_BONI; ++i)
    {
        const auto& b = ability_db[id].attribute_bonus[i];
        if (b.attribute > 0 && b.displayed == 1)
        {
            if (!text.empty())
                text += "; ";
            text += ability_db[b.attribute].name_display + "+" + str(b.bonus);
            if (ability_db[b.attribute].percentage == 1)
                text += "%";
        }
    }
    return text;
}

int ability_capped(int id, int bonus)
{
    int cap = attribute_cap(id);
    return cap > -1 && current_character.ability[id].attribute_max_value + bonus >= cap;
}

int ability_available(int id)
{
    int rc = 1;
    for (int i = 0; i < MAX_NUMBER_OF_ABILITY_REQUIREMENTS; ++i)
    {
        int attr = ability_db[id].ability_requirement[i].attribute;
        int req = ability_db[id].ability_requirement[i].value;
        if (attr > 0 && current_character.ability[attr].attribute_max_value < req)
            rc = 0;
    }
    return rc;
}

// ================================================================ screens
void character_info_screen()
{
    auto& p = P();
    draw_standard_frame(message_list[MESSAGE_CHARACTER_INFO], message_list[MESSAGE_PRESS_RETURN_TO_LEAVE]);
    struct T
    {
        int x, y, tile;
    };
    std::vector<T> tiles;
    int row = 0, column = 0, column_width = 160;
    p.setFont(pb::FONT_SMALL);
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
    {
        int cur = attribute_current_val(i);
        for (int j = 0; j < MAX_NUMBER_OF_ABILITY_DISPLAYS; ++j)
        {
            const auto& dsp = ability_db[i].display[j];
            if (dsp.type == 2 &&
                ((dsp.start_value <= cur && dsp.end_value >= cur) || (dsp.start_value == 0 && dsp.end_value == 0)))
            {
                std::string text = replace_string(dsp.message, "[value]", str(cur));
                if (dsp.expanded_value_display == 1)
                    text = replace_string(dsp.message, "[value]", ability_expanded_value_display(i));
                p.text((float)(56 + column * column_width), (float)(52 + row * 32), text, dsp.color);
                if (!dsp.message2.empty())
                {
                    text = replace_string(dsp.message2, "[value]", str(cur));
                    if (dsp.expanded_value_display == 1)
                        text = replace_string(dsp.message2, "[value]", ability_expanded_value_display(i));
                    p.text((float)(56 + column * column_width), (float)(68 + row * 32), text, dsp.color);
                }
                tiles.push_back({20 + column * column_width, 52 + row * 32, ability_db[i].tile});
                if (++row > 10)
                {
                    row = 0;
                    ++column;
                }
            }
        }
    }
    for (auto& t : tiles)
        draw_sheet_tile(SPRITE_ABILITIES, t.tile, (float)t.x, (float)t.y, false);
    const auto& sh = p.sheet(sheets[SPRITE_SILHOUETTE]);
    int half = sh.w / 2;
    p.blit(sheets[SPRITE_SILHOUETTE], silhouette ? half : 0, 0, half, sh.h, 390, 80, 255, true, (float)(half / 4),
           (float)(sh.h / 4));
}

int abilities_available_for_xp()
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (!ability_db[i].name.empty() && ability_db[i].xp == 1)
            ++rc;
    return rc;
}

int experience_screen(int selected_entry, int number_of_entries, int offset, const std::string& title,
                      uint32_t selected_entry_color)
{
    auto& p = P();
    int rc = 0, frame = 0, gain_power_tile_y = 0, selected_ability_id = 0, bonus2 = 0;
    const int column_width = 300;
    draw_standard_frame(title);
    for (int i = 3; i <= 28; ++i)
    {
        p.blit(sheets[SPRITE_Frame], 0, frame * 16 + 16, 16, 16, 320, i * 16.f, 255, true);
        if (++frame > 4)
            frame = 0;
    }
    if (selected_entry - offset <= EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN)
        p.box(20, (float)(EXPERIENCE_SCREEN_ABILITIES_TOP_BORDER + (selected_entry - offset) * 32), column_width - 8.f, 32,
              selected_entry_color);
    int row = -offset;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (!ability_db[i].name.empty() && ability_db[i].xp == 1)
        {
            if (row >= 0 && row <= EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN)
            {
                draw_sheet_tile(SPRITE_ABILITIES, ability_db[i].tile, 20,
                                (float)(EXPERIENCE_SCREEN_ABILITIES_TOP_BORDER + row * 32));
                if (row == selected_entry - offset)
                {
                    rc = i;
                    draw_sheet_tile(SPRITE_ABILITIES, ability_db[i].tile, 338, 50, false);
                }
            }
            ++row;
        }
    row = -offset;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
    {
        if (ability_db[i].name.empty() || ability_db[i].xp != 1)
            continue;
        if (row >= 0 && row <= EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN)
        {
            std::string text = ability_db[i].name_display;
            if (ability_available(i) == 0)
                text += " (" + message_list[MESSAGE_NOT_AVAILABLE] + ")";
            else if (ability_capped(i) == 1)
                text += " (" + message_list[MESSAGE_CAPPED] + ")";
            p.setFont(pb::FONT_VERDANA_BOLD);
            text = shorten_text(text, column_width - 48);
            const float ry = (float)(EXPERIENCE_SCREEN_ABILITIES_TOP_BORDER + row * 32);
            p.text((float)EXPERIENCE_SCREEN_ABILITIES_LEFT_BORDER, ry, text, ability_db[i].color);
            if (row == selected_entry - offset)
                p.text(372, 58, ability_db[i].name_display, ability_db[i].color);
            p.setFont(pb::FONT_VERDANA);
            text = shorten_text(ability_bonus_text(i), column_width - 48);
            p.text((float)EXPERIENCE_SCREEN_ABILITIES_LEFT_BORDER, ry + 16, text, ability_db[i].color);
            if (row == selected_entry - offset)
            {
                int text_y = 82;
                bonus2 = 0;
                selected_ability_id = i;
                for (int j = 0; j < MAX_NUMBER_OF_ATTRIBUTE_BONI; ++j)
                {
                    int attribute = ability_db[i].attribute_bonus[j].attribute;
                    int bonus = ability_db[i].attribute_bonus[j].bonus;
                    if (attribute == i)
                        bonus2 = bonus;
                    if (attribute > 0 && ability_db[i].attribute_bonus[j].displayed == 1)
                    {
                        text = ability_db[attribute].name_display + "+" + str(bonus);
                        if (ability_db[attribute].percentage == 1)
                            text += "%";
                        text += " (" + message_list[MESSAGE_CURRENT_VALUE] + ": " +
                                str(current_character.ability[attribute].attribute_max_value) + ")";
                        p.text(340, (float)text_y, shorten_text(text, 300), ability_db[attribute].color);
                        text_y += 16;
                    }
                }
                for (int j = 0; j < MAX_NUMBER_OF_ABILITY_CAPS; ++j)
                {
                    const auto& cp = ability_db[i].ability_cap[j];
                    if (cp.ability > 0)
                    {
                        text = ability_db[cp.ability].name_display;
                        int cap = current_character.ability[cp.ability].attribute_max_value;
                        if (cp.multiplier > 1)
                        {
                            text += " x " + str(cp.multiplier);
                            cap = cap * cp.multiplier;
                        }
                        else if (cp.divisor > 1)
                        {
                            text += " / " + str(cp.divisor);
                            cap = cap / cp.divisor;
                        }
                        if (cp.bonus > 0)
                        {
                            text += " + " + str(cp.bonus);
                            cap += cp.bonus;
                        }
                        text = message_list[MESSAGE_CAPPED_AT] + " " + str(cap) + " (" + text + ")";
                        p.text(340, (float)text_y, shorten_text(text, 300), ability_db[cp.ability].color);
                        text_y += 16;
                    }
                }
                for (int j = 0; j < MAX_NUMBER_OF_ABILITY_REQUIREMENTS; ++j)
                {
                    const auto& rq = ability_db[i].ability_requirement[j];
                    if (rq.attribute > 0)
                    {
                        text = message_list[MESSAGE_REQUIRED] + ": " + ability_db[rq.attribute].name_display + " " +
                               str(rq.value) + "+";
                        p.text(340, (float)text_y, shorten_text(text, 300), ability_db[rq.attribute].color);
                        text_y += 16;
                    }
                }
                if (!ability_db[i].description.empty())
                {
                    text_y += wrap_text(340, text_y, ability_db[i].description, 240, 12, 0xffffff) * 12;
                    text_y += 16;
                }
                for (int j = 0; j < MAX_NUMBER_OF_POWERS; ++j)
                    if (power_available(j) == 0 && power_available(j, i, bonus2) == 1)
                    {
                        if (gain_power_tile_y == 0)
                            gain_power_tile_y = text_y;
                        p.text(378, (float)text_y, message_list[MESSAGE_NEW_POWER] + ":", rgb(255, 100, 0));
                        text_y += 12;
                        p.text(378, (float)text_y, shorten_text(power_db[j].name, 300), 0xffffff);
                        text_y += 26;
                    }
            }
        }
        ++row;
    }
    if (number_of_entries > EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN)
    {
        p.box(300, (float)EXPERIENCE_SCREEN_ABILITIES_TOP_BORDER, 14, 384, rgb(125, 75, 0));
        p.box(301, EXPERIENCE_SCREEN_ABILITIES_TOP_BORDER + 1.f, 12, 382, 0);
        float bar_entry_height = 384.f / (float)imax(1, number_of_entries - 1);
        int bar_y = (int)(offset * bar_entry_height) + EXPERIENCE_SCREEN_ABILITIES_TOP_BORDER + 2;
        int bar_height = (int)(bar_entry_height * EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN) - 2;
        p.box(302, (float)bar_y, 10, (float)bar_height, rgb(255, 150, 0));
        p.line(302, (float)bar_y, 302, (float)(bar_y + bar_height - 2), rgb(255, 200, 125));
        p.line(303, (float)bar_y, 311, (float)bar_y, rgb(255, 200, 125));
        p.line(311, (float)bar_y, 311, (float)(bar_y + bar_height - 2), rgb(125, 75, 0));
        p.line(303, (float)(bar_y + bar_height - 1), 311, (float)(bar_y + bar_height - 1), rgb(125, 75, 0));
    }
    if (gain_power_tile_y > 0)
        for (int j = 0; j < MAX_NUMBER_OF_POWERS; ++j)
            if (power_available(j) == 0 && power_available(j, selected_ability_id, bonus2) == 1)
            {
                draw_sheet_tile(SPRITE_POWERS, power_db[j].tile, 340, (float)gain_power_tile_y, false);
                gain_power_tile_y += 38;
            }
    return rc;
}

void experience(int experience_points, int game_start)
{
    auto& p = P();
    int remaining_xp = experience_points;
    bool key_lock = true, esc = false;
    int selected_entry = 0, offset = 0, mouse_x = 0, mouse_y = 0;
    int selected_ability_id = 0;
    bool mouse_over_entry = false;
    do
    {
        p.examineKeyboard();
        if (!p.keyPushed(pb::K_ALL))
            key_lock = false;
        else
            p.delayMs(5);
    } while (key_lock);
    p.takeClick();
    while (remaining_xp > 0 && !esc)
    {
        bool finished = false;
        std::string title = game_start == 1 ? message_list[MESSAGE_CHOOSE_STARTING_ABILITIES]
                                            : message_list[MESSAGE_YOU_GAIN_SOME_EXPERIENCE];
        title += " (" + str(remaining_xp) + ")";
        int number_of_entries = abilities_available_for_xp();
        selected_ability_id = experience_screen(selected_entry, number_of_entries, offset, title);
        p.flip();
        while (!finished && !esc)
        {
            bool update_screen = false;
            p.examineKeyboard();
            bool clicked = p.takeClick();
            int mouse_x_new = p.mouseX(), mouse_y_new = p.mouseY();
            bool mouse_moved = mouse_x_new != mouse_x || mouse_y_new != mouse_y;
            mouse_x = mouse_x_new;
            mouse_y = mouse_y_new;
            if (p.keyReleased(pb::K_ESCAPE))
            {
                finished = true;
                esc = true;
            }
            if (p.keyPushed(pb::K_DOWN) && !key_lock && selected_entry < number_of_entries - 1)
            {
                ++selected_entry;
                if (selected_entry > EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN)
                    offset = selected_entry - EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN;
                update_screen = true;
                key_lock = true;
            }
            if (p.keyPushed(pb::K_UP) && !key_lock && selected_entry > 0)
            {
                --selected_entry;
                if (selected_entry < EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN)
                    offset = 0;
                update_screen = true;
                key_lock = true;
            }
            int wheel = p.mouseWheel();
            if (wheel < 0 && offset < number_of_entries - EXPERIENCE_SCREEN_ABILITIES_ON_SCREEN - 1)
            {
                ++offset;
                update_screen = true;
                key_lock = true;
                mouse_moved = true;
            }
            else if (wheel > 0 && offset > 0)
            {
                --offset;
                update_screen = true;
                key_lock = true;
                mouse_moved = true;
            }
            if (mouse_moved)
            {
                mouse_over_entry = false;
                if (mouse_x >= EXPERIENCE_SCREEN_ABILITIES_LEFT_BORDER && mouse_x <= 300 && mouse_y >= 50 &&
                    mouse_y <= 432)
                {
                    selected_entry = (mouse_y - 50) / 32 + offset;
                    update_screen = true;
                    key_lock = true;
                    mouse_over_entry = true;
                }
            }
            if (selected_entry < offset)
                selected_entry = offset;
            if (selected_entry > number_of_entries - 1)
                selected_entry = imax(0, number_of_entries - 1);
            if ((p.keyPushed(pb::K_RETURN) && !key_lock) || (clicked && mouse_over_entry))
            {
                if (selected_ability_id > 0 && selected_ability_id < MAX_NUMBER_OF_ABILITIES - 1)
                {
                    if (ability_available(selected_ability_id) == 1)
                    {
                        for (int i = 0; i < MAX_NUMBER_OF_ATTRIBUTE_BONI; ++i)
                        {
                            int attribute = ability_db[selected_ability_id].attribute_bonus[i].attribute;
                            int bonus = ability_db[selected_ability_id].attribute_bonus[i].bonus;
                            if (attribute > 0)
                            {
                                auto& a = current_character.ability[attribute];
                                a.attribute_max_value += bonus;
                                a.attribute_current_value += bonus;
                                a.ability_value += 1;
                            }
                            experience_screen(selected_entry, number_of_entries, offset, title, rgb(255, 100, 0));
                            p.flip();
                        }
                        finished = true;
                    }
                }
                update_abilities_adjustment_other();
                key_lock = true;
                update_screen = true;
            }
            if (p.keyReleased(pb::K_ALL))
                key_lock = false;
            if (update_screen)
            {
                selected_ability_id = experience_screen(selected_entry, number_of_entries, offset, title);
                p.flip();
            }
            else
            {
                p.delayMs(10);
            }
        }
        --remaining_xp;
    }
}

void delete_savegame()
{
    namespace fs = std::filesystem;
    std::error_code ec;
    for (auto& e : fs::directory_iterator(save_path(""), ec))
        if (e.is_regular_file() && e.path().extension() == ".xml")
            fs::remove(e.path(), ec);
}

// ================================================================ attribute manipulation
int restore_attribute(int id)
{
    int rc = 0;
    int cur = attribute_current_val(id), mx = attribute_max_val(id);
    if (cur < mx)
    {
        current_character.ability[id].attribute_current_value = mx;
        rc = 1;
    }
    update_abilities_adjustment_other();
    return rc;
}

void reduce_ability(int id, int value, int negative_value_allowed)
{
    auto& a = current_character.ability[id];
    a.attribute_current_value -= value;
    if (ability_db[id].minimum != -1)
        a.attribute_current_value = imax(a.attribute_current_value, ability_db[id].minimum);
    if (negative_value_allowed == 0)
        a.attribute_current_value = imax(a.attribute_current_value, 0);
    update_abilities_adjustment_other();
}

int increase_attribute(int id, int value)
{
    int rc = value;
    auto& a = current_character.ability[id];
    if (ability_db[id].depletable == 1)
        rc = imin(a.attribute_max_value - a.attribute_current_value, value);
    a.attribute_current_value += rc;
    update_abilities_adjustment_other();
    return rc;
}

int modify_attribute(int id, int value)
{
    int rc = value;
    if (id < 1 || id >= MAX_NUMBER_OF_ABILITIES)
        error_message("modify_attribute(): wrong attribute id " + str(id));
    if (value > 0)
        rc = increase_attribute(id, value);
    else
        reduce_ability(id, std::abs(value));
    update_abilities_adjustment_other();
    return rc;
}

int modify_buffer_attribute(int id, int value)
{
    int rc = value;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].buffer_for_attribute == id && id > 0 && rc > 0)
            rc = imax(0, rc - modify_attribute(i, rc));
    return rc;
}

int character_died()
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].game_ends > 0)
        {
            int cur = attribute_current_val(i);
            if (ability_db[i].game_ends == 1 && cur < ability_db[i].game_ends_val)
                rc = 1;
            if (ability_db[i].game_ends == 2 && cur > ability_db[i].game_ends_val)
                rc = 1;
            if (rc == 1 && !ability_db[i].game_ends_message.empty() && current_character.game_end_message.empty())
                current_character.game_end_message = ability_db[i].game_ends_message;
        }
    return rc;
}

int random_damage_in_melee()
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].damage_in_melee == 1)
        {
            rc += attribute_current_val(i);
            if (ability_db[i].randomizer_ability > 0)
                rc += rnd(attribute_current_val(ability_db[i].randomizer_ability));
        }
    return rc;
}

int defense_in_melee()
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].defense_melee == 1)
            rc += attribute_current_val(i);
    return rc;
}

int character_damage_buffer(int damage)
{
    int rc = std::abs(damage);
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].modify_per_point_of_damage != 0 && rc > 0)
            for (int j = 1; j < MAX_NUMBER_OF_ABILITIES; ++j)
                if (ability_db[j].buffer_for_attribute == i && rc > 0)
                    if (attribute_current_val(j) > 0)
                    {
                        play_sound("hit");
                        animation(ability_db[j].buffer_animation, 6, 6, rc);
                        rc = imax(0, rc - std::abs(modify_attribute(j, -rc)));
                    }
    return rc;
}

void character_takes_damage(int value, const std::string& game_end_message)
{
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].modify_per_point_of_damage != 0)
            modify_attribute(i, value * ability_db[i].modify_per_point_of_damage);
    if (!game_end_message.empty() && character_died() == 1)
        current_character.game_end_message = game_end_message;
    update_abilities_adjustment_other();
}

int sneaking_val(int only_when_activated)
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].sneaking == 1 && (only_when_activated == 0 || current_character.ability[i].activated == 1))
            rc += attribute_current_val(i);
    return rc;
}

int backstabbing_val(int only_when_activated)
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].backstab == 1 && (only_when_activated == 0 || current_character.ability[i].activated == 1))
            rc += attribute_current_val(i);
    return rc;
}

int perception_val(const std::string& tile_class)
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (!ability_db[i].perception_tile_class.empty())
            if (compare_string_lists(ability_db[i].perception_tile_class, tile_class) ||
                ability_db[i].perception_tile_class == "all")
                rc += attribute_current_val(i);
    return rc;
}

void update_abilities_adjustment_other()
{
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
    {
        current_character.ability[i].adjustment_other = 0;
        current_character.ability[i].adjustment_percentage = 100;
    }
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
    {
        int cur = attribute_current_val(i);
        if (cur <= 0)
            continue;
        for (int j = 0; j < MAX_NUMBER_OF_ABILITY_ADJUSTMENTS; ++j)
        {
            const auto& adj = ability_db[i].adjustment[j];
            int attribute = adj.attribute;
            if (attribute <= 0)
                continue;
            auto& tgt = current_character.ability[attribute];
            if (adj.value != 0)
                tgt.adjustment_other += adj.value;
            if (adj.percentage != 100)
                tgt.adjustment_percentage = (tgt.adjustment_percentage * adj.percentage) / 100;
            if (adj.percentage_attribute_multiplier > 0)
                tgt.adjustment_percentage =
                    (tgt.adjustment_percentage * ((adj.percentage_attribute_multiplier * cur) + adj.percentage_bonus)) / 100;
        }
    }
}

int movement_left()
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].movement == 1)
            rc += attribute_current_val(i);
    return rc;
}

int armor_protection()
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].armor == 1)
            rc += attribute_current_val(i);
    return rc;
}

int random_armor_protection()
{
    int rc = armor_protection();
    if (rc > 0)
        rc = rnd(rc);
    return rc;
}

}  // namespace ll
