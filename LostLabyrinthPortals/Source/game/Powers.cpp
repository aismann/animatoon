// powers.pb
#include "game/Game.h"

#include <cstdlib>

namespace ll
{

using pb::str;
using pb::val;
using pb::XmlDoc;

void reset_power(power_db_struct& p)
{
    p = power_db_struct{};
    p.strength_divisor = 0;
    p.duration_multiplier = 0;
    p.duration_divisor = 0;
}

static int find_ability(const std::string& name)
{
    int rc = 0;
    for (int k = 0; k < MAX_NUMBER_OF_ABILITIES; ++k)
        if (ability_db[k].name == name)
            rc = k;
    return rc;
}

void powers_db_resolve_names()
{
    for (auto& p : power_db)
    {
        auto res = [](int& id, const std::string& name)
        {
            if (id == 0 && !name.empty())
                id = find_ability(name);
        };
        res(p.strength_bonus_attribute, p.strength_bonus_attribute_name);
        res(p.randomized_strength_bonus_attribute, p.randomized_strength_bonus_attribute_name);
        res(p.success_attribute, p.success_attribute_name);
        res(p.cost_attribute, p.cost_attribute_name);
        res(p.modify_attribute, p.modify_attribute_name);
        res(p.adjust_attribute, p.adjust_attribute_name);
        res(p.duration_bonus_attribute, p.duration_bonus_attribute_name);
        res(p.activates_ability_id, p.activates_ability_name);
        res(p.deplete_attribute_id, p.deplete_attribute_name);
        for (auto& r : p.power_requirement)
            res(r.attribute, r.attribute_name);
    }
}

void load_power_db()
{
    for (auto& p : power_db)
        reset_power(p);
    XmlDoc d;
    if (!d.load(res_path("data/powers.xml")))
        error_message("load_power_db(): could not open data/powers.xml: " + d.error);
    for (auto& n : d.root->children)
    {
        if (n->name != "power")
            continue;
        power_db_struct np;
        reset_power(np);
        int id = 0;
        auto yes = [](const std::string& v) { return v == "yes" ? 1 : 0; };
        for (auto& a : n->attrs)
        {
            const std::string& k = a.first;
            const std::string& v = a.second;
            if (k == "id") id = (int)val(v);
            else if (k == "name") np.name = v;
            else if (k == "description") np.description = v;
            else if (k == "tile") np.tile = (int)val(v);
            else if (k == "effect") np.effect = (int)val(v);
            else if (k == "effect_name") np.effect_name = v;
            else if (k == "effect_strength") np.effect_strength = (int)val(v);
            else if (k == "strength_bonus_attribute") np.strength_bonus_attribute = (int)val(v);
            else if (k == "strength_bonus_attribute_name") np.strength_bonus_attribute_name = v;
            else if (k == "strength_multiplier") np.strength_multiplier = (int)val(v);
            else if (k == "strength_divisor") np.strength_divisor = (int)val(v);
            else if (k == "strength_randomized") np.strength_randomized = yes(v);
            else if (k == "randomized_strength_bonus") np.randomized_strength_bonus = (int)val(v);
            else if (k == "randomized_strength_bonus_attribute") np.randomized_strength_bonus_attribute = (int)val(v);
            else if (k == "randomized_strength_bonus_attribute_name") np.randomized_strength_bonus_attribute_name = v;
            else if (k == "randomized_strength_multiplier") np.randomized_strength_multiplier = (int)val(v);
            else if (k == "randomized_strength_divisor") np.randomized_strength_divisor = (int)val(v);
            else if (k == "animation") np.animation = v;
            else if (k == "end_turn") np.end_turn = yes(v);
            else if (k == "sound") np.sound = v;
            else if (k == "failure_sound") np.failure_sound = v;
            else if (k == "wrong_use_sound") np.wrong_use_sound = v;
            else if (k == "success_attribute") np.success_attribute = (int)val(v);
            else if (k == "success_attribute_name") np.success_attribute_name = v;
            else if (k == "success_bonus") np.success_bonus = (int)val(v);
            else if (k == "success_attribute_multiplier") np.success_attribute_multiplier = (int)val(v);
            else if (k == "success_attribute_divisor") np.success_attribute_divisor = (int)val(v);
            else if (k == "success_message") np.success_message = v;
            else if (k == "failure_message") np.failure_message = v;
            else if (k == "wrong_use_message") np.wrong_use_message = v;
            else if (k == "already_used_message") np.already_used_message = v;
            else if (k == "cost") np.cost = (int)val(v);
            else if (k == "cost_attribute") np.cost_attribute = (int)val(v);
            else if (k == "cost_attribute_name") np.cost_attribute_name = v;
            else if (k == "class") np.class_ = v;
            else if (k == "range") np.range = (int)val(v);
            else if (k == "modify_attribute") np.modify_attribute = (int)val(v);
            else if (k == "modify_attribute_name") np.modify_attribute_name = v;
            else if (k == "adjust_attribute") np.adjust_attribute = (int)val(v);
            else if (k == "adjust_attribute_name") np.adjust_attribute_name = v;
            else if (k == "duration") np.duration = (int)val(v);
            else if (k == "duration_bonus_attribute") np.duration_bonus_attribute = (int)val(v);
            else if (k == "duration_bonus_attribute_name") np.duration_bonus_attribute_name = v;
            else if (k == "duration_multiplier") np.duration_multiplier = (int)val(v);
            else if (k == "duration_divisor") np.duration_divisor = (int)val(v);
            else if (k == "activates_ability_id") np.activates_ability_id = (int)val(v);
            else if (k == "activates_ability_name") np.activates_ability_name = v;
            else if (k == "deplete_attribute_id") np.deplete_attribute_id = (int)val(v);
            else if (k == "deplete_attribute_name") np.deplete_attribute_name = v;
            else if (k == "cooldown") np.cooldown = (int)val(v);
            else if (k == "monster_effect") np.monster_effect = v;
            else if (k == "affects_only_monster_class") np.affects_only_monster_class = v;
            else if (k == "field_effect") np.field_effect = v;
        }
        if (id < 1 || id >= MAX_NUMBER_OF_POWERS)
            error_message("load_power_db(): wrong power id " + str(id));
        int req_idx = 0;
        for (auto& c : n->children)
        {
            std::string lang = c->attrOr("language");
            if (c->name == "name")
            {
                if (lang == preferences.language)
                    np.name = c->text;
            }
            else if (c->name == "description")
            {
                if (lang == preferences.language)
                    np.description = c->text;
            }
            else if (c->name == "requirement")
            {
                power_requirement_struct r;
                for (auto& a : c->attrs)
                {
                    if (a.first == "attribute") r.attribute = (int)val(a.second);
                    else if (a.first == "attribute_name") r.attribute_name = a.second;
                    else if (a.first == "value") r.value = (int)val(a.second);
                }
                if (req_idx < MAX_NUMBER_OF_POWER_REQUIREMENTS)
                    np.power_requirement[req_idx++] = r;
            }
            else if (c->name == "message")
            {
                if (lang == preferences.language)
                {
                    std::string t = c->attrOr("type");
                    if (t == "success") np.success_message = c->text;
                    else if (t == "failure") np.failure_message = c->text;
                    else if (t == "wrong use") np.wrong_use_message = c->text;
                    else if (t == "already used") np.already_used_message = c->text;
                }
            }
        }
        power_db[id] = np;
    }
    powers_db_resolve_names();
}

// ================================================================ strength, chance, messages
int power_strength(int id)
{
    const auto& p = power_db[id];
    int rc = p.effect_strength;
    int attr = p.strength_bonus_attribute;
    if (attr > 0 && attr < MAX_NUMBER_OF_ABILITIES)
    {
        int bonus = attribute_current_val(attr);
        if (p.strength_multiplier != 1)
            bonus *= p.strength_multiplier;
        if (p.strength_divisor > 1)
            bonus /= p.strength_divisor;
        rc += bonus;
    }
    return rc;
}

static int power_strength_random_part(int id)
{
    const auto& p = power_db[id];
    int rc = 0;
    if (p.strength_randomized == 1)
    {
        rc = p.randomized_strength_bonus;
        int attr = p.randomized_strength_bonus_attribute;
        if (attr > 0 && attr < MAX_NUMBER_OF_ABILITIES)
        {
            int bonus = attribute_current_val(attr);
            if (p.randomized_strength_multiplier != 1)
                bonus *= p.randomized_strength_multiplier;
            if (p.randomized_strength_divisor > 1)
                bonus /= p.randomized_strength_divisor;
            rc += bonus;
        }
    }
    return rc;
}

static int power_strength_randomized(int id)
{
    int rc = power_strength_random_part(id);
    if (rc > 0)
        rc = rnd(rc);
    return rc + power_strength(id);
}

static void power_cooldown(int id)
{
    if (power_db[id].cooldown > 0)
        current_character.power[id].cooldown = power_db[id].cooldown;
}

static int power_success_sound(int id)
{
    if (power_db[id].sound.empty())
        return 0;
    play_sound(power_db[id].sound);
    return 1;
}
static int power_failure_sound(int id)
{
    if (power_db[id].failure_sound.empty())
        return 0;
    play_sound(power_db[id].failure_sound);
    return 1;
}
static int power_wrong_usage_sound(int id)
{
    if (power_db[id].wrong_use_sound.empty())
        return 0;
    play_sound(power_db[id].wrong_use_sound);
    return 1;
}
static int power_success_message(int id)
{
    if (power_db[id].success_message.empty())
        return 0;
    message(replace_string(power_db[id].success_message, "[power_strength]", str(power_strength(id)), true), "",
            0xffffff, power_db[id].tile, 2);
    return 1;
}
static int power_msg(const std::string& m, int id)
{
    if (m.empty())
        return 0;
    message(m, "", 0xffffff, power_db[id].tile, 2);
    return 1;
}
static int power_failure_message(int id)
{
    return power_msg(power_db[id].failure_message, id);
}
static int power_already_used_message(int id)
{
    return power_msg(power_db[id].already_used_message, id);
}
static int power_wrong_usage_message(int id)
{
    return power_msg(power_db[id].wrong_use_message, id);
}

int power_available(int id, int gain_attribute, int gain_bonus)
{
    if (power_db[id].name.empty())
        return 0;
    int rc = 1;
    for (int i = 0; i < MAX_NUMBER_OF_POWER_REQUIREMENTS; ++i)
    {
        int attribute = power_db[id].power_requirement[i].attribute;
        int value = power_db[id].power_requirement[i].value;
        if (attribute > 0)
        {
            int cur = attribute_current_val(attribute, 1, 0, 0);
            if (attribute == gain_attribute)
                cur += gain_bonus;
            if (cur < value)
                rc = 0;
        }
    }
    return rc;
}

static int power_success_chance(int id)
{
    const auto& p = power_db[id];
    int rc = 100;
    int attr = p.success_attribute;
    if (attr > 0 && attr < MAX_NUMBER_OF_ABILITIES)
    {
        rc = attribute_current_val(attr);
        if (p.success_attribute_multiplier > 1)
            rc *= p.success_attribute_multiplier;
        if (p.success_attribute_divisor > 1)
            rc /= p.success_attribute_divisor;
        rc += p.success_bonus;
    }
    return rc;
}

// ================================================================ effects
static void draw_hit_scene(int x1, int y1, int damage)
{
    draw_game_scene();
    P().blitFull(sheets[SPRITE_BLOOD], (float)x1, (float)y1);
    draw_text_centered_number(x1, y1, str(damage));
}

static void power_attack(int id)
{
    cursor_struct cursor;
    reset_cursor(cursor);
    cursor_select_field(cursor);
    monster_struct* target = nullptr;
    if (cursor.esc != 1)
        for (auto& m : monster)
            if (m.xpos == current_character.xpos - 6 + cursor.x && m.ypos == current_character.ypos - 6 + cursor.y)
                target = &m;
    if (target)
    {
        animation(power_db[id].animation, cursor.x, cursor.y);
        if (rnd(99) >= power_success_chance(id))
        {
            target = nullptr;
            power_failure_sound(id);
            power_failure_message(id);
        }
    }
    if (target)
    {
        int damage = power_strength_randomized(id);
        damage = monster_adjust_damage(target->type, damage, power_db[id].class_, power_db[id].affects_only_monster_class);
        power_success_message(id);
        play_sound("hit");
        draw_hit_scene(cursor.x * 32, cursor.y * 32, damage);
        target->hitpoints -= damage;
        kill_monster();
        P().flip();
        P().delayMs(250);
        draw_game_scene();
        P().flip();
        P().delayMs(250);
    }
}

static void power_area_attack(int id)
{
    animation(power_db[id].animation);
    for (auto& m : monster)
    {
        if (m.xpos >= current_character.xpos - 6 && m.xpos <= current_character.xpos + 6 &&
            m.ypos >= current_character.ypos - 6 && m.ypos <= current_character.ypos + 6)
        {
            int damage = power_strength_randomized(id);
            damage = monster_adjust_damage(m.type, damage, power_db[id].class_, power_db[id].affects_only_monster_class);
            play_sound("hit");
            animation("red cloud", m.xpos + 6 - current_character.xpos, m.ypos + 6 - current_character.ypos, damage);
            m.hitpoints -= damage;
        }
    }
    kill_monster();
}

static void power_duration_effect(int id)
{
    const auto& p = power_db[id];
    if (p.adjust_attribute <= 0)
        return;
    if (rnd(99) >= power_success_chance(id))
    {
        power_failure_sound(id);
        power_failure_message(id);
        return;
    }
    int attribute = p.adjust_attribute;
    int adjustment = power_strength(id);
    int duration = p.duration, bonus = 0;
    if (p.duration_bonus_attribute > 0)
        bonus = attribute_current_val(p.duration_bonus_attribute);
    if (p.duration_multiplier > 1)
        bonus *= p.duration_multiplier;
    if (p.duration_divisor > 1)
        bonus /= p.duration_divisor;
    auto& a = current_character.ability[attribute];
    a.power_adjustment = adjustment;
    a.adjustment_duration = duration + bonus;
    a.adjustment_name = p.name;
    a.adjustment_power_id = id;
    a.adjustment_item_id = 0;
    animation(p.animation);
    power_success_sound(id);
    power_success_message(id);
    power_cooldown(id);
}

static int power_modify_attribute(int id, int secondary_effect = 0)
{
    int rc = 0;
    if (power_db[id].modify_attribute > 0)
    {
        if (secondary_effect == 0)
        {
            animation(power_db[id].animation);
            power_success_sound(id);
            power_success_message(id);
        }
        rc = modify_attribute(power_db[id].modify_attribute, power_strength(id));
        if (rc != 0 && secondary_effect == 0)
            power_cooldown(id);
    }
    return rc;
}

static int power_use_field(int id, int transformation)
{
    const auto& p = power_db[id];
    cursor_struct cursor;
    reset_cursor(cursor);
    cursor_select_field(cursor, p.range);
    int x = current_character.xpos - 6 + cursor.x, y = current_character.ypos - 6 + cursor.y;
    int tile_type = read_map_tile_type(current_map, x, y);
    const std::string cls = current_map.tileset.tile_type_db[tile_type].class_;
    int transforms_into = current_map.tileset.tile_type_db[tile_type].transforms_into;
    int chance = power_success_chance(id);
    int rc = 1;
    if (!compare_string_lists(p.class_, cls))
    {
        power_wrong_usage_sound(id);
        power_wrong_usage_message(id);
        rc = 0;
    }
    if (rc == 1 && read_map_if_power_used(current_map, x, y, p.field_effect) == 1)
    {
        power_wrong_usage_sound(id);
        power_already_used_message(id);
        rc = 0;
    }
    if (rc == 1)
    {
        animation(p.animation, cursor.x, cursor.y);
        if (rnd(99) >= chance)
        {
            power_failure_sound(id);
            power_failure_message(id);
            add_map_power_used(current_map, x, y, p.field_effect);
            rc = 0;
        }
    }
    if (rc == 1)
    {
        if (transformation == 1)
            set_map_tile_type(current_map, x, y, transforms_into);
        power_success_sound(id);
        power_success_message(id);
        power_modify_attribute(id, 1);
        power_cooldown(id);
        add_map_power_used(current_map, x, y, p.field_effect);
    }
    return rc;
}

static void power_activate_ability(int id)
{
    int aid = power_db[id].activates_ability_id;
    if (aid > 0)
    {
        current_character.ability[aid].activated = current_character.ability[aid].activated == 1 ? 0 : 1;
        power_cooldown(id);
    }
}

static void power_deplete_attribute(int id)
{
    int aid = power_db[id].deplete_attribute_id;
    if (aid > 0)
    {
        animation(power_db[id].animation);
        power_success_sound(id);
        power_success_message(id);
        update_abilities_adjustment_other();
        current_character.ability[aid].attribute_current_value = 0;
    }
}

static int power_apply_effect_to_monster(int id)
{
    const auto& p = power_db[id];
    int rc = 0;
    cursor_struct cursor;
    reset_cursor(cursor);
    cursor_select_field(cursor);
    monster_struct* target = nullptr;
    if (cursor.esc != 1)
        for (auto& m : monster)
            if (m.xpos == current_character.xpos - 6 + cursor.x && m.ypos == current_character.ypos - 6 + cursor.y)
            {
                rc = 1;
                target = &m;
            }
    auto fail = [&](Msg m)
    {
        animation(p.animation, cursor.x, cursor.y);
        play_sound("failure");
        message(message_list[m], "", 0xffffff, p.tile, 2);
        rc = 0;
    };
    if (rc == 1 && compare_string_lists(target->effect, p.monster_effect))
        fail(MESSAGE_MONSTER_ALREADY_AFFECTED_BY_POWER);
    if (rc == 1 && compare_string_lists(monster_type_db[target->type].immune_class, p.class_))
        fail(MESSAGE_MONSTER_IS_IMMUNE_VS_POWER);
    if (rc == 1 && compare_string_lists(monster_type_db[target->type].resistance_class, p.class_) && rnd(3) > 0)
        fail(MESSAGE_MONSTER_HAS_RESISTED);
    if (rc == 1)
    {
        if (p.monster_effect == "sleep")
            target->alerted = 0;
        else
        {
            if (!target->effect.empty())
                target->effect += "|";
            target->effect += p.monster_effect;
            target->alerted = 1;
        }
        animation(p.animation, cursor.x, cursor.y);
        power_success_sound(id);
        power_success_message(id);
        update_abilities_adjustment_other();
    }
    return rc;
}

void use_power_effect(int id)
{
    const std::string& e = power_db[id].effect_name;
    if (e == "attack") power_attack(id);
    else if (e == "area attack") power_area_attack(id);
    else if (e == "duration") power_duration_effect(id);
    else if (e == "transform field") power_use_field(id, 1);
    else if (e == "use field") power_use_field(id, 0);
    else if (e == "activate ability") power_activate_ability(id);
    else if (e == "deplete attribute") power_deplete_attribute(id);
    else if (e == "modify attribute") power_modify_attribute(id);
    else if (e == "apply effect to monster") power_apply_effect_to_monster(id);
}

// ================================================================ selection screen
static void use_power_screen(power_selection_struct& ps)
{
    auto& p = P();
    p.clearScreen(0);
    draw_map_screen(current_map);
    draw_character();
    int sh = sheets[SPRITE_Frame], frame = 1;
    p.blit(sh, 0, 6 * 16, 16, 16, 0, 26 * 16);
    for (int i = 1; i <= 39; ++i)
    {
        p.blit(sh, 0, 7 * 16 + frame * 16, 16, 16, i * 16.f, 26 * 16.f, 255, true);
        if (++frame > 4)
            frame = 0;
    }
    p.blit(sh, 0, 0, 16, 16, 13 * 32.f, 0, 255, true);
    frame = 1;
    for (int i = 1; i <= 25; ++i)
    {
        p.blit(sh, 0, frame * 16 + 16, 16, 16, 13 * 32.f, i * 16.f, 255, true);
        if (++frame > 4)
            frame = 0;
    }
    p.setFont(pb::FONT_VERDANA_BOLD);
    p.text(416 + 16 + 2, 0, message_list[MESSAGE_USE_A_POWER] + ":", 0xffffff);
    p.setFont(pb::FONT_VERDANA);
    int sid = ps.selected_power_id;
    if (sid > 0)
    {
        const auto& pw = power_db[sid];
        p.text(434, 240, pw.name, rgb(0, 255, 255));
        int y = 256;
        if (pw.success_attribute > 0)
        {
            p.text(434, (float)y, message_list[MESSAGE_SUCCESS_CHANCE] + ": " + str(power_success_chance(sid)) + "%",
                   rgb(255, 100, 0));
            y += 16;
        }
        if (pw.cost > 0)
        {
            p.text(434, (float)y, message_list[MESSAGE_COST] + ": " + str(pw.cost) + " " + pw.cost_attribute_name,
                   rgb(255, 100, 0));
            y += 16;
        }
        std::string t = message_list[MESSAGE_RANGE] + ": ";
        t += " " + (pw.range == 0 ? message_list[MESSAGE_RANGE_SELF]
                                  : (pw.range == 1 ? message_list[MESSAGE_RANGE_TOUCH] : message_list[MESSAGE_RANGE_SIGHT]));
        p.text(434, (float)y, t, rgb(255, 100, 0));
        y += 16;
        if (current_character.power[sid].cooldown > 0)
        {
            p.text(434, (float)y,
                   replace_string(message_list[MESSAGE_COOLDOWN], "[cooldown]", str(current_character.power[sid].cooldown)),
                   rgb(255, 100, 0));
            y += 16;
        }
        p.setFont(pb::FONT_SMALL);
        wrap_text(434, y, pw.description, 640 - 434, 12);
    }
    int power_idx = 1;
    ps.selected_power_id = 0;
    int cols = tile_columns[SPRITE_POWERS];
    for (int y = 0; y <= 5; ++y)
        for (int x = 0; x <= 5; ++x)
        {
            int tx = 0, ty = 0;
            bool found = false;
            while (!found && power_idx < MAX_NUMBER_OF_POWERS - 1)
            {
                if (power_available(power_idx))
                {
                    int tile = power_db[power_idx].tile;
                    ty = tile / cols;
                    tx = tile - ty * cols;
                    found = true;
                    if (ps.cursor.x == x && ps.cursor.y == y)
                        ps.selected_power_id = power_idx;
                }
                ++power_idx;
            }
            p.blit(sheets[SPRITE_POWERS], tx * 32, ty * 32, 32, 32, 434.f + x * 32, 32.f + y * 32, 255, true);
            if (ps.cursor.x == x && ps.cursor.y == y)
                p.blit(sheets[SPRITE_CURSOR], ps.cursor.frame * 32, 0, 32, 32, 434.f + x * 32, 32.f + y * 32);
        }
}

int use_power()
{
    auto& p = P();
    power_selection_struct ps;
    reset_cursor(ps.cursor);
    ps.cursor.x = ps.cursor.y = 0;
    bool key_lock = true, leave = false, esc = false;
    do
    {
        p.examineKeyboard();
        if (p.keyReleased(pb::K_ALL))
            key_lock = false;
        p.delayMs(5);
    } while (key_lock);
    while (!leave)
    {
        p.examineKeyboard();
        if (p.keyReleased(pb::K_ESCAPE))
        {
            leave = true;
            esc = true;
        }
        if (p.keyPushed(pb::K_LEFT) && !key_lock && ps.cursor.x > 0)
        {
            --ps.cursor.x;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_RIGHT) && !key_lock && ps.cursor.x < 5)
        {
            ++ps.cursor.x;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_UP) && !key_lock && ps.cursor.y > 0)
        {
            --ps.cursor.y;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_DOWN) && !key_lock && ps.cursor.y < 5)
        {
            ++ps.cursor.y;
            key_lock = true;
        }
        if (p.keyReleased(pb::K_RETURN))
            leave = true;
        if (p.keyReleased(pb::K_ALL))
            key_lock = false;
        blink_cursor(ps.cursor);
        use_power_screen(ps);
        p.flip();
    }
    int id = ps.selected_power_id;
    int rc = 0;
    if (!esc && id > 0)
    {
        int cost = power_db[id].cost, cost_attr = power_db[id].cost_attribute;
        if (cost > 0)
        {
            if (attribute_current_val(cost_attr) < cost)
            {
                play_sound("failure");
                message(replace_string(message_list[MESSAGE_INSUFFICIENT_POWER_RESOURCE], "[attribute_name]",
                                       ability_db[cost_attr].name_display),
                        message_list[MESSAGE_INSUFFICIENT_POWER_RESOURCE2], 0xffffff, power_db[id].tile, 2);
                esc = true;
            }
            else
                reduce_ability(cost_attr, cost, 0);
        }
    }
    if (!esc && id > 0 && power_db[id].cooldown > 0 && current_character.power[id].cooldown > 0)
    {
        play_sound("failure");
        message(replace_string(message_list[MESSAGE_COOLDOWN_TOO_HIGH], "[cooldown]",
                               str(current_character.power[id].cooldown)),
                "", 0xffffff, power_db[id].tile, 2);
        esc = true;
    }
    if (!esc && id > 0)
    {
        use_power_effect(id);
        if (power_db[id].end_turn == 1)
            rc = 1;
    }
    return rc;
}

}  // namespace ll
