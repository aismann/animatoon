// monster.pb
#include "game/Game.h"

#include <cstdlib>

namespace ll
{

using pb::str;
using pb::val;
using pb::XmlDoc;

void reset_monster_type(monster_type_db_struct& m)
{
    m = monster_type_db_struct{};
}

void load_monster_db()
{
    for (auto& m : monster_type_db)
        reset_monster_type(m);
    XmlDoc d;
    if (!d.load(res_path("data/monster.xml")))
        error_message("load_monster_db(): could not open data/monster.xml: " + d.error);
    for (auto& n : d.root->children)
    {
        if (n->name != "monster")
            continue;
        int id = 0;
        monster_type_db_struct nm;
        for (auto& a : n->attrs)
        {
            const std::string& k = a.first;
            const std::string& v = a.second;
            if (k == "id") id = (int)val(v);
            else if (k == "name") nm.name = v;
            else if (k == "class") nm.class_ = v;
            else if (k == "tile") nm.tile = (int)val(v);
            else if (k == "attack") nm.attack = (int)val(v);
            else if (k == "defense") nm.defense = (int)val(v);
            else if (k == "strength") nm.strength = (int)val(v);
            else if (k == "hitpoints") nm.hitpoints = (int)val(v);
            else if (k == "armor") nm.armor = (int)val(v);
            else if (k == "level") nm.level = (int)val(v);
            else if (k == "speed") nm.speed = (int)val(v);
            else if (k == "flying") nm.flying = v == "yes";
            else if (k == "regeneration") nm.regeneration = (int)val(v);
            else if (k == "resistance") nm.resistance_class = v;
            else if (k == "immune") nm.immune_class = v;
            else if (k == "vulnerable") nm.vulnerable_class = v;
        }
        if (id < 1 || id >= MAX_NUMBER_OF_MONSTER_TYPES || nm.name.empty())
            error_message("load_monster_db(): invalid monster entry id=" + str(id));
        monster_type_db[id] = nm;
        int aff_idx = 0;
        for (auto& c : n->children)
        {
            if (c->name == "name")
            {
                if (c->attrOr("language") == preferences.language)
                    monster_type_db[id].name = c->text;
            }
            else if (c->name == "affliction")
            {
                monster_affliction_struct af;
                af.chance = 100;
                for (auto& a : c->attrs)
                {
                    const std::string& k = a.first;
                    const std::string& v = a.second;
                    if (k == "class") af.class_ = v;
                    else if (k == "chance") af.chance = (int)val(v);
                    else if (k == "attribute") af.attribute = (int)val(v);
                    else if (k == "attribute_name") af.attribute_name = v;
                    else if (k == "value") af.value = (int)val(v);
                    else if (k == "randomized") af.randomized = v == "yes";
                    else if (k == "animation") af.animation = v;
                    else if (k == "sound") af.sound = v;
                    else if (k == "message") af.message = v;
                }
                if (aff_idx >= MAX_NUMBER_OF_MONSTER_AFFLICTIONS)
                    error_message("load_monster_db(): too many afflictions for monster " + str(id));
                monster_type_db[id].affliction[aff_idx] = af;
                for (auto& g : c->children)
                    if (g->name == "message" && g->attrOr("language") == preferences.language)
                        monster_type_db[id].affliction[aff_idx].message = g->text;
                ++aff_idx;
            }
        }
    }
    // resolve attribute names
    for (auto& m : monster_type_db)
        for (auto& af : m.affliction)
            if (af.attribute == 0 && !af.attribute_name.empty())
                for (int k = 0; k < MAX_NUMBER_OF_ABILITIES; ++k)
                    if (ability_db[k].name == af.attribute_name)
                        af.attribute = k;
}

int field_contains_monster(int x, int y)
{
    for (auto& m : monster)
        if (m.xpos == x && m.ypos == y)
            return 1;
    return 0;
}

int add_monster(map_struct&, const monster_struct& nm)
{
    if (nm.xpos >= 0 && nm.xpos < MAP_DIMENSION_X && nm.ypos >= 0 && nm.ypos < MAP_DIMENSION_Y)
    {
        for (auto& m : monster)
            if (m.xpos == nm.xpos && m.ypos == nm.ypos)
                return 0;
        monster_struct m;
        m.type = nm.type;
        m.xpos = nm.xpos;
        m.ypos = nm.ypos;
        m.hitpoints = nm.hitpoints;
        m.alerted = nm.alerted;
        m.y_offset = nm.y_offset;
        m.effect = nm.effect.empty() ? "" : nm.effect;
        monster.push_back(m);
        return 1;
    }
    return 0;
}

void clear_selected_monster()
{
    for (auto& m : monster)
        m.selected = 0;
}

int kill_monster(int kill_selected)
{
    int rc = 0;
    for (auto it = monster.begin(); it != monster.end();)
    {
        if ((it->selected == 1 && kill_selected == 1) || it->hitpoints < 1)
        {
            it = monster.erase(it);
            ++rc;
            for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
                if (ability_db[i].modify_with_each_kill > 0)
                    modify_attribute(i, ability_db[i].modify_with_each_kill);
        }
        else
            ++it;
    }
    return rc;
}

void character_attacks_monster(map_struct&, int x, int y)
{
    monster_struct* target = nullptr;
    for (auto& m : monster)
    {
        m.selected = 0;
        if (m.xpos == x && m.ypos == y)
        {
            target = &m;
            m.selected = 1;
        }
    }
    if (!target)
        return;
    int chance_to_hit = 0, damage = 0, hit = 0, critical_hit = 0, backstab_attack = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (ability_db[i].attack_melee == 1)
            chance_to_hit = attribute_current_val(i);
    chance_to_hit -= monster_type_db[target->type].defense;
    if (target->alerted == 0 && backstabbing_val(1) > 0)
    {
        chance_to_hit = sneaking_val(0);
        backstab_attack = 1;
    }
    chance_to_hit = imax(10, imin(90, chance_to_hit));
    target->alerted = 1;
    if (rnd(99) <= chance_to_hit)
    {
        hit = 1;
        play_sound("hit");
        damage = imax(0, random_damage_in_melee() - monster_type_db[target->type].armor);
        if (backstab_attack == 1)
        {
            damage += backstabbing_val(0);
            message(replace_string(message_list[MESSAGE_SUCCESSFUL_BACKSTAB], "[damage]", str(damage)), "");
        }
        if (damage > 0)
            for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
            {
                if (ability_db[i].hit_multiplier > 0)
                    modify_attribute(i, ability_db[i].hit_multiplier);
                if (ability_db[i].hit_attribute > 0)
                    modify_attribute(i, attribute_current_val(ability_db[i].hit_attribute));
                if (ability_db[i].critical_hit_melee == 1 && rnd(99) <= attribute_current_val(i))
                {
                    critical_hit = 1;
                    damage = target->hitpoints;
                }
            }
    }
    else
        play_sound("evasion");

    P().clearScreen(0);
    draw_map_screen(current_map);
    draw_frame_main_screen();
    draw_right_panel();
    draw_character(-2);
    if (hit == 1)
    {
        int x1 = (6 + x - current_character.xpos) * 32, y1 = (6 + y - current_character.ypos) * 32;
        if (critical_hit == 1)
            draw_sheet_tile(SPRITE_FX, SPRITE_FX_BLOOD_SKULL, (float)x1, (float)y1);
        else
        {
            draw_sheet_tile(SPRITE_FX, SPRITE_FX_CLOUD_RED, (float)x1, (float)y1);
            draw_text_centered_number(x1, y1, str(damage));
        }
        target->hitpoints -= damage;
    }
    P().flip();
    P().delayMs(250);
    kill_monster();
    draw_game_scene();
    P().flip();
    P().delayMs(250);
}

void monster_attacks_character(int x, int y)
{
    monster_struct* att = nullptr;
    for (auto& m : monster)
        if (m.xpos == x && m.ypos == y)
        {
            att = &m;
            m.y_offset = -2;
        }
    if (!att)
        return;
    int hit = 0, damage = 0;
    const auto& mt = monster_type_db[att->type];
    att->alerted = 1;
    int chance_to_hit = imax(10, imin(90, mt.attack - defense_in_melee()));
    if (rnd(99) <= chance_to_hit)
    {
        hit = 1;
        play_sound("hit");
        damage = rnd(mt.strength + 2) + 1;
        damage = character_damage_buffer(damage);
        damage = imax(0, damage - random_armor_protection());
        character_takes_damage(damage, message_list[MESSAGE_YOU_WERE_KILLED_BY] + " " + mt.name);
        for (int i = 0; i < MAX_NUMBER_OF_MONSTER_AFFLICTIONS; ++i)
        {
            const auto& af = mt.affliction[i];
            if (af.chance > 0 && af.attribute > 0 && damage > 0 && rnd(99) < af.chance)
            {
                int value = af.value;
                if (af.randomized == 1)
                {
                    value = rnd(std::abs(value) - 1) + 1;
                    if (af.value < 0)
                        value = -value;
                }
                if (value != 0)
                    modify_attribute(af.attribute, value);
            }
        }
    }
    else
        play_sound("evasion");
    att->y_offset = -2;
    draw_game_scene();
    if (hit == 1)
    {
        draw_sheet_tile(SPRITE_FX, SPRITE_FX_CLOUD_RED, 6 * 32.f, 6 * 32.f);
        draw_text_centered_number(6 * 32, 6 * 32, str(damage));
    }
    P().flip();
    P().delayMs(250);
    att->y_offset = 0;
    draw_game_scene();
    P().flip();
    P().delayMs(250);
}

int random_monster_type(int max_level)
{
    std::vector<int> list;
    for (int i = 1; i < MAX_NUMBER_OF_MONSTER_TYPES; ++i)
        if (!monster_type_db[i].name.empty() && monster_type_db[i].level <= max_level)
            list.push_back(i);
    if (list.empty())
        return 0;
    return list[rnd((int)list.size() - 1)];
}

int place_random_monster(map_struct& m, int max_level)
{
    int type = random_monster_type(max_level);
    int x = 0, y = 0, stop_counter = 0;
    bool found;
    do
    {
        x = rnd(MAP_DIMENSION_X - 1);
        y = rnd(MAP_DIMENSION_Y - 1);
        ++stop_counter;
        found = field_is_blocked_for_monster(m, x, y, type) != 1;
    } while (!(found && stop_counter < 100) && stop_counter < 100);
    if (stop_counter < 100)
    {
        monster_struct nm;
        nm.type = type;
        nm.xpos = x;
        nm.ypos = y;
        nm.hitpoints = monster_type_db[type].hitpoints;
        return add_monster(m, nm);
    }
    return 0;
}

int spawn_random_monster(map_struct& m)
{
    if (m.spawn_monster_chance > 0 && m.spawn_monster_level > 0 && rnd(99) <= m.spawn_monster_chance)
    {
        int top = 0;
        for (int i = 0; i < MAX_NUMBER_OF_MONSTER_TYPES; ++i)
            if (!monster_type_db[i].name.empty())
                top = i;
        if (top > 0)
        {
            int type = 0, guard = 0;
            do
            {
                type = rnd(top);
            } while (monster_type_db[type].level != m.spawn_monster_level && ++guard < 10000);
            if (guard >= 10000)
                return 0;
            int x, y;
            guard = 0;
            do
            {
                x = rnd(MAP_DIMENSION_X - 1);
                y = rnd(MAP_DIMENSION_Y - 1);
            } while (!(field_is_blocked(m, x, y) == 0 && field_contains_monster(x, y) == 0) && ++guard < 10000);
            if (guard >= 10000)
                return 0;
            monster_struct nm;
            nm.type = type;
            nm.xpos = x;
            nm.ypos = y;
            nm.hitpoints = monster_type_db[type].hitpoints;
            return add_monster(m, nm);
        }
    }
    return 0;
}

static int sgn(int v)
{
    return v < 0 ? -1 : 1;
}

void move_all_monster()
{
    int char_x = current_character.xpos, char_y = current_character.ypos;
    for (auto it = monster.begin(); it != monster.end(); ++it)
    {
        monster_struct& mo = *it;
        int monster_type = mo.type;
        int speed = monster_type_db[monster_type].speed;
        if (compare_string_lists(mo.effect, "slowed"))
            speed = 1;
        while (speed > 0)
        {
            int dx = 0, dy = 0;
            if (mo.alerted == 1)
            {
                if (char_x != mo.xpos || char_y != mo.ypos)
                {
                    if (std::abs(char_y - mo.ypos) > std::abs(char_x - mo.xpos))
                        dy = sgn(current_character.ypos - mo.ypos);
                    else
                        dx = sgn(current_character.xpos - mo.xpos);
                    if (mo.xpos + dx == char_x && mo.ypos + dy == char_y)
                    {
                        speed = 0;
                        dx = dy = 0;
                    }
                }
            }
            else
                speed = 0;
            if (dx != 0 || dy != 0)
            {
                bool random_move = false;
                if (field_is_blocked_for_monster(current_map, mo.xpos + dx, mo.ypos + dy, monster_type) == 1)
                    random_move = true;
                if (compare_string_lists(mo.effect, "confused"))
                    random_move = true;
                if (random_move)
                {
                    int direction = rnd(3), count = 0;
                    bool found = false;
                    dx = dy = 0;
                    while (count < 4 && !found)
                    {
                        switch (direction)
                        {
                        case 0:
                            if (field_is_blocked_for_monster(current_map, mo.xpos, mo.ypos - 1, monster_type) == 0)
                            {
                                dy = -1;
                                found = true;
                            }
                            break;
                        case 1:
                            if (field_is_blocked_for_monster(current_map, mo.xpos + 1, mo.ypos, monster_type) == 0)
                            {
                                dx = 1;
                                found = true;
                            }
                            break;
                        case 2:
                            if (field_is_blocked_for_monster(current_map, mo.xpos, mo.ypos + 1, monster_type) == 0)
                            {
                                dy = 1;
                                found = true;
                            }
                            break;
                        case 3:
                            if (field_is_blocked_for_monster(current_map, mo.xpos - 1, mo.ypos, monster_type) == 0)
                            {
                                dx = -1;
                                found = true;
                            }
                            break;
                        }
                        if (++direction > 3)
                            direction = 0;
                        ++count;
                    }
                    if (!found)
                        speed = 0;
                }
            }
            if (speed > 0 && (dx != 0 || dy != 0))
            {
                if (mo.xpos >= current_character.xpos - 7 && mo.xpos <= current_character.xpos + 7 &&
                    mo.ypos >= current_character.ypos - 7 && mo.ypos <= current_character.ypos + 7)
                {
                    for (int i = 2; i <= 32; i += 2)
                    {
                        mo.x_offset = i * dx;
                        mo.y_offset = i * dy;
                        draw_game_scene();
                        P().flip();
                    }
                    P().delayMs(250);
                }
                mo.x_offset = 0;
                mo.y_offset = 0;
                mo.xpos += dx;
                mo.ypos += dy;
            }
            --speed;
        }
    }
}

void monster_turn_ends()
{
    for (auto& mo : monster)
    {
        int type = mo.type;
        const auto& mt = monster_type_db[type];
        if (mt.regeneration > 0 && mo.hitpoints < mt.hitpoints)
        {
            int value = imin(rnd(mt.regeneration - 1) + 1, mt.hitpoints - mo.hitpoints);
            if (mo.xpos >= current_character.xpos - 6 && mo.xpos <= current_character.xpos + 6 &&
                mo.ypos >= current_character.ypos - 6 && mo.ypos <= current_character.ypos + 6)
            {
                message(replace_string(message_list[MESSAGE_MONSTER_REGENERATES], "[value]", str(value)), "", 0xffffff,
                        mt.tile, 3);
                star_circle_animation(4, mo.xpos - current_character.xpos + 6, mo.ypos - current_character.ypos + 6);
            }
            mo.hitpoints = imin(mo.hitpoints + value, mt.hitpoints);
        }
    }
}

int alert_monster(int x, int y)
{
    int rc = 0;
    for (auto& m : monster)
        if (m.xpos == x && m.ypos == y)
        {
            m.alerted = 1;
            rc = 1;
        }
    return rc;
}

int monster_is_alerted(int x, int y)
{
    int rc = 0;
    for (auto& m : monster)
        if (m.xpos == x && m.ypos == y && m.alerted == 1)
            rc = 1;
    return rc;
}

int monster_adjust_damage(int monster_type, int damage, const std::string& damage_class, const std::string& affects_only)
{
    // faithful to the original: only the "resistance" halving reaches the return value
    int rc = damage;
    const auto& mt = monster_type_db[monster_type];
    if (compare_string_lists(mt.resistance_class, damage_class))
        rc = rc / 2;
    if (compare_string_lists(mt.vulnerable_class, damage_class))
        damage *= 2;
    if (compare_string_lists(mt.immune_class, damage_class))
        damage = 0;
    if (!affects_only.empty() && !compare_string_lists(mt.class_, affects_only))
        damage = 0;
    return rc;
}

}  // namespace ll
