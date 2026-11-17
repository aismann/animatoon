// items.pb
#include "game/Game.h"

#include <cstdio>
#include <cstdlib>

namespace ll
{

using pb::str;
using pb::val;
using pb::XmlDoc;

// ================================================================ database
static void reset_item(item_db_struct& it)
{
    it = item_db_struct{};
    it.tile_equipped = -1;
    it.identified = 1;
}

void item_db_resolve_names()
{
    for (int i = 1; i < MAX_NUMBER_OF_ITEMS; ++i)
    {
        auto& it = item_db[i];
        for (auto& b : it.ability_bonus)
            if (!b.attribute_name.empty() && b.attribute == 0)
                for (int k = 1; k < MAX_NUMBER_OF_ABILITIES; ++k)
                    if (ability_db[k].name == b.attribute_name)
                        b.attribute = k;
        if (!it.power_name.empty() && it.power_id == 0)
            for (int j = 1; j < MAX_NUMBER_OF_POWERS; ++j)
                if (power_db[j].name == it.power_name)
                    it.power_id = j;
    }
}

void scramble_item_db_tiles(const std::string& cls)
{
    struct E
    {
        int item, tile;
    };
    std::vector<E> items;
    for (int i = 0; i < MAX_NUMBER_OF_ITEMS; ++i)
        if (item_db[i].class_ == cls)
            items.push_back({i, item_db[i].tile});
    int count = (int)items.size();
    if (count > 1)
    {
        for (int index = 0; index < count; ++index)
        {
            int r = index;
            while (r == index)
                r = rnd(count - 1);
            std::swap(items[index].tile, items[r].tile);
        }
        for (auto& e : items)
            item_db[e.item].tile = e.tile;
    }
}

void load_item_db()
{
    for (auto& it : item_db)
        it = item_db_struct{};
    XmlDoc d;
    if (!d.load(res_path("data/items.xml")))
        error_message("load_item_db(): could not open data/items.xml: " + d.error);
    for (auto& n : d.root->children)
    {
        if (n->name != "item")
            continue;
        item_db_struct ni;
        reset_item(ni);
        int id = 0;
        auto yes = [](const std::string& v) { return v == "yes" ? 1 : 0; };
        for (auto& a : n->attrs)
        {
            const std::string& k = a.first;
            const std::string& v = a.second;
            if (k == "id") id = (int)val(v);
            else if (k == "name") ni.name = v;
            else if (k == "tile") ni.tile = (int)val(v);
            else if (k == "tile_equipped") ni.tile_equipped = (int)val(v);
            else if (k == "weight") ni.weight = (int)val(v);
            else if (k == "starting_equipment") ni.starting_equipment = (int)val(v);
            else if (k == "class") ni.class_ = v;
            else if (k == "can_be_equipped") ni.can_be_equipped = yes(v);
            else if (k == "description") ni.description = v;
            else if (k == "price") ni.price = (int)val(v);
            else if (k == "fuel") ni.fuel = (int)val(v);
            else if (k == "fuel_consumed_per_turn") ni.fuel_consumed_per_turn = (int)val(v);
            else if (k == "fuel_name") ni.fuel_name = v;
            else if (k == "fuel_randomized") ni.fuel_randomized = yes(v);
            else if (k == "use_name") ni.use_name = v;
            else if (k == "consumed_after_use") ni.consumed_after_use = yes(v);
            else if (k == "turn_ends_after_use") ni.turn_ends_after_use = yes(v);
            else if (k == "use_message") ni.use_message = v;
            else if (k == "fuel_consumed_per_use") ni.fuel_consumed_per_use = (int)val(v);
            else if (k == "artefact") ni.artefact = yes(v);
            else if (k == "equipment_limit") ni.equipment_limit = (int)val(v);
            else if (k == "identified") ni.identified = yes(v);
            else if (k == "identified_when_used") ni.identified_when_used = yes(v);
            else if (k == "use_animation") ni.use_animation = v;
            else if (k == "power_id") ni.power_id = (int)val(v);
            else if (k == "power_name") ni.power_name = v;
        }
        if (id < 1 || id >= MAX_NUMBER_OF_ITEMS || ni.name.empty())
            error_message("load_item_db(): invalid item entry id=" + str(id));
        ni.class_name = ni.class_;
        int bonus_idx = 0, unequip_idx = 0;
        for (auto& c : n->children)
        {
            bool lang_ok = c->attrOr("language") == preferences.language;
            if (c->name == "ability_bonus")
            {
                item_bonus_struct b;
                for (auto& a : c->attrs)
                {
                    const std::string& k = a.first;
                    const std::string& v = a.second;
                    if (k == "attribute") b.attribute = (int)val(v);
                    else if (k == "attribute_name") b.attribute_name = v;
                    else if (k == "value") b.value = (int)val(v);
                    else if (k == "magic_bonus_multiplier") b.magic_bonus_multiplier = (int)val(v);
                    else if (k == "randomized") b.randomized = yes(v);
                    else if (k == "randomized_bonus") b.randomized_bonus = (int)val(v);
                    else if (k == "duration") b.duration = (int)val(v);
                }
                if (bonus_idx >= MAX_NUMBER_OF_ITEM_ABILITY_BONI)
                    error_message("load_item_db(): too many ability_bonus tags for item " + ni.name);
                ni.ability_bonus[bonus_idx++] = b;
            }
            else if (c->name == "unequip")
            {
                if (unequip_idx >= MAX_NUMBER_OF_ITEM_UNEQUIP_CLASSES)
                    error_message("load_item_db(): too many unequip tags for item " + ni.name);
                ni.unequip_item_class[unequip_idx++] = c->attrOr("class");
            }
            else if (c->name == "name") { if (lang_ok) ni.name = c->text; }
            else if (c->name == "description") { if (lang_ok) ni.description = c->text; }
            else if (c->name == "class_name") { if (lang_ok) ni.class_name = c->text; }
            else if (c->name == "fuel_name") { if (lang_ok) ni.fuel_name = c->text; }
            else if (c->name == "message")
            {
                if (lang_ok && c->attrOr("type") == "use")
                    ni.use_message = c->text;
            }
        }
        item_db[id] = ni;
    }
    item_db_resolve_names();
}

// ================================================================ equipment
int number_of_equipped_items(const std::string& cls)
{
    int rc = 0;
    for (auto& it : inventory)
        if (it.equipped == 1 && item_db[it.type].class_ == cls)
            ++rc;
    return rc;
}

static void unequip_item_class(int item_type)
{
    for (auto& it : inventory)
        for (int i = 0; i < MAX_NUMBER_OF_ITEM_UNEQUIP_CLASSES; ++i)
            if (!item_db[item_type].unequip_item_class[i].empty() &&
                item_db[item_type].unequip_item_class[i] == item_db[it.type].class_)
                it.equipped = 0;
}

void update_equipment_boni()
{
    for (auto& a : current_character.ability)
        a.item_bonus = 0;
    for (auto& it : inventory)
        if (it.equipped == 1)
            for (int i = 0; i < MAX_NUMBER_OF_ITEM_ABILITY_BONI; ++i)
            {
                const auto& ab = item_db[it.type].ability_bonus[i];
                int bonus = ab.value;
                if (ab.magic_bonus_multiplier > 0)
                    bonus += it.magic_bonus * ab.magic_bonus_multiplier;
                if (ab.attribute > 0)
                    current_character.ability[ab.attribute].item_bonus += bonus;
            }
}

// ================================================================ inventory screen
static void draw_side_panel_frame()
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
}

static std::string weight_text(int weight)
{
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.1f", (double)(weight / 10));  // integer division, as in the original
    return buf;
}

static void inventory_screen(cursor_struct& cursor, menu_struct& item_menu, int row_offset, int y_pixel_offset = 0)
{
    auto& p = P();
    int items_in_inventory = (int)inventory.size();
    draw_side_panel_frame();
    for (auto& it : inventory)
        it.selected = 0;
    // visible cells
    auto iter = inventory.begin();
    int skip = row_offset * INVENTORY_ROWS;
    for (int i = 0; i < skip && iter != inventory.end(); ++i)
        ++iter;
    for (int cell = 0; cell < INVENTORY_ROWS * INVENTORY_COLUMNS; ++cell)
    {
        int x = cell % INVENTORY_COLUMNS, y = cell / INVENTORY_COLUMNS;
        float px = 434.f + x * 32, py = 32.f + y * 32 + y_pixel_offset;
        bool cursor_here = cursor.x == x && cursor.y == y && y_pixel_offset == 0;
        if (iter != inventory.end() && skip >= 0)
        {
            auto& it = *iter;
            if (it.equipped == 1)
                draw_sheet_tile(SPRITE_ITEMS, TILE_ITEM_EQUIPPED, px, py, false);
            else
                draw_sheet_tile(SPRITE_FX, SPRITE_FX_GREY_BOX, px, py, false);
            if (it.equipped == 1 && item_db[it.type].tile_equipped >= 0)
                draw_sheet_tile(SPRITE_ITEMS, item_db[it.type].tile_equipped, px, py);
            else
                draw_sheet_tile(SPRITE_ITEMS, item_db[it.type].tile, px, py);
            if (cursor_here)
            {
                it.selected = 1;
                draw_sheet_tile(SPRITE_ITEMS, item_db[it.type].tile, 434, 240, false);
                p.blit(sheets[SPRITE_CURSOR], cursor.frame * 32, 0, 32, 32, px, py);
            }
            ++iter;
        }
        else
        {
            draw_sheet_tile(SPRITE_FX, SPRITE_FX_GREY_BOX, px, py, false);
            if (cursor_here)
                p.blit(sheets[SPRITE_CURSOR], cursor.frame * 32, 0, 32, 32, px, py);
        }
    }
    if (y_pixel_offset < 0)
        p.box(434, 0, INVENTORY_COLUMNS * 32.f, 32, 0);
    if (y_pixel_offset > 0)
        p.box(434, INVENTORY_ROWS * 32.f + 32, INVENTORY_COLUMNS * 32.f, 32, 0);
    p.setFont(pb::FONT_DEFAULT);
    p.text(416 + 16 + 2, 0,
           message_list[MESSAGE_INVENTORY] + " (" + str(items_in_inventory) + " " + message_list[MESSAGE_ITEMS] + ")",
           0xffffff);
    for (auto& it : inventory)
    {
        if (it.selected != 1)
            continue;
        int type = it.type;
        const auto& db = item_db[type];
        int y = 240;
        std::string text = db.name;
        if (db.artefact == 1)
            text += " + " + str(it.magic_bonus);
        bool known = db.identified == 1 || current_character.item_identification[type].identified == 1;
        if (!known)
            text = message_list[MESSAGE_UNIDENTIFIED_ITEM];
        if (wrap_text(468, y, text, 640 - 468, 16, rgb(0, 255, 255)) < 2)
            p.text(468, (float)(y + 16), db.class_name, rgb(255, 100, 0));
        y += 32;
        p.setFont(pb::FONT_VERDANA);
        if (it.amount > 1)
        {
            p.text(434, (float)y, message_list[MESSAGE_AMOUNT] + ": " + str(it.amount), rgb(255, 100, 0));
            y += 16;
        }
        if (known)
        {
            for (int i = 0; i < MAX_NUMBER_OF_ITEM_ABILITY_BONI; ++i)
            {
                const auto& ab = db.ability_bonus[i];
                int bonus = ab.value;
                if (db.artefact == 1)
                    bonus += ab.magic_bonus_multiplier * it.magic_bonus;
                if (ab.attribute > 0)
                {
                    std::string t = ability_db[ab.attribute].name_display;
                    if (bonus > 0)
                        t += " + ";
                    if (ab.randomized == 1)
                        t += str(ab.randomized_bonus) + " - " + str(bonus + ab.randomized_bonus);
                    else
                        t += str(bonus);
                    if (ability_db[ab.attribute].percentage == 1)
                        t += "%";
                    p.text(434, (float)y, shorten_text(t, 170), rgb(0, 255, 0));
                    y += 16;
                }
            }
            if (db.fuel > 0 && !db.fuel_name.empty())
            {
                p.text(434, (float)y, shorten_text(replace_string(db.fuel_name, "[fuel]", str(it.fuel)), 170),
                       rgb(255, 0, 255));
                y += 16;
            }
            p.text(434, (float)y,
                   shorten_text(message_list[MESSAGE_WEIGHT] + ": " + weight_text(db.weight) + " " + message_list[MESSAGE_POUND],
                                170),
                   rgb(255, 0, 0));
            y += 16;
            p.setFont(pb::FONT_SMALL);
            wrap_text(434, y, db.description, 640 - 434, 12, 0xffffff);
        }
    }
    if (items_in_inventory > 36)
    {
        p.box(626, 32, 14, 192, 0x777777);
        p.box(627, 33, 12, 190, 0);
        int total_rows = (items_in_inventory + INVENTORY_COLUMNS - 1) / INVENTORY_COLUMNS;
        int row_px = (INVENTORY_ROWS * 32) / total_rows;
        int bar_y = 34 + row_offset * row_px;
        int bar_h = row_px * INVENTORY_ROWS - 1;
        p.box(628, (float)bar_y, 10, (float)bar_h, 0x777777);
        p.line(628, (float)bar_y, 637, (float)bar_y, 0xdddddd);
        p.line(628, (float)bar_y, 628, (float)(bar_y + bar_h), 0xdddddd);
        p.line(637, bar_y + 1.f, 637, (float)(bar_y + bar_h), 0x222222);
        p.line(629, (float)(bar_y + bar_h), 637, (float)(bar_y + bar_h), 0x222222);
    }
    item_menu.x = 434 + cursor.x * 32 + 16;
    item_menu.y = 32 + cursor.y * 32 + 16;
    display_menu(item_menu);
}

static item_in_inventory_struct* selected_item()
{
    item_in_inventory_struct* r = nullptr;
    for (auto& it : inventory)
        if (it.selected == 1)
            r = &it;
    return r;
}

void open_inventory()
{
    auto& p = P();
    cursor_struct cursor;
    reset_cursor(cursor);
    cursor.x = current_character.inventory_cursor_x;
    cursor.y = current_character.inventory_cursor_y;
    menu_struct item_menu;
    reset_menu(item_menu);
    add_menu_entry(item_menu, message_list[MESSAGE_DROP]);
    add_menu_entry(item_menu, message_list[MESSAGE_CANCEL]);
    item_menu.font = pb::FONT_SMALL;
    menu_auto_set(item_menu);
    item_menu.spacing = 0;
    item_menu.padding = 0;
    item_menu.menu_open = 0;
    item_menu.entry_border = 0;
    item_menu.border = 1;
    bool key_lock = true, leave = false;
    int row_offset = 0;
    int item_type = 0;
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
        bool menu_open = item_menu.menu_open == 1;
        if (p.keyReleased(pb::K_ESCAPE))
            leave = true;
        if (p.keyPushed(pb::K_LEFT) && !key_lock && cursor.x > 0 && !menu_open)
        {
            --cursor.x;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_RIGHT) && !key_lock && cursor.x < INVENTORY_COLUMNS - 1 && !menu_open)
        {
            ++cursor.x;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_UP) && !key_lock && cursor.y > 0 && !menu_open)
        {
            --cursor.y;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_UP) && !key_lock && cursor.y == 0 && !menu_open && row_offset > 0)
        {
            for (int i = 2; i <= 30; i += 2)
            {
                inventory_screen(cursor, item_menu, row_offset, i);
                p.flip();
            }
            --row_offset;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_UP) && !key_lock && menu_open)
        {
            if (--item_menu.selected_entry < 0)
                item_menu.selected_entry = item_menu.number_of_entries - 1;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_DOWN) && !key_lock && cursor.y < INVENTORY_ROWS - 1 && !menu_open)
        {
            ++cursor.y;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_DOWN) && !key_lock && cursor.y == INVENTORY_ROWS - 1 && !menu_open)
            if (row_offset < ((int)inventory.size() - 31) / 6)
            {
                for (int i = 2; i <= 30; i += 2)
                {
                    inventory_screen(cursor, item_menu, row_offset, -i);
                    p.flip();
                }
                ++row_offset;
                key_lock = true;
            }
        if (p.keyPushed(pb::K_DOWN) && !key_lock && menu_open)
        {
            if (++item_menu.selected_entry >= item_menu.number_of_entries)
                item_menu.selected_entry = 0;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_RETURN) && item_menu.menu_open == 0 && !key_lock)
        {
            bool found = false;
            if (auto* it = selected_item())
            {
                item_type = it->type;
                menu_reset_entries(item_menu);
                std::string use = item_db[item_type].use_name;
                if (item_db[item_type].fuel_consumed_per_use > 0 && it->fuel < 1)
                    use.clear();
                if (!use.empty())
                    add_menu_entry(item_menu, item_db[item_type].use_name);
                int can_equip = item_db[item_type].can_be_equipped == 1;
                if (item_db[item_type].fuel > 0 && it->fuel == 0)
                    can_equip = 0;
                if (can_equip)
                    add_menu_entry(item_menu, it->equipped == 1 ? message_list[MESSAGE_ITEM_MENU_UNEQUIP]
                                                                : message_list[MESSAGE_ITEM_MENU_EQUIP]);
                add_menu_entry(item_menu, message_list[MESSAGE_DROP]);
                if (it->amount > 1)
                    add_menu_entry(item_menu, message_list[MESSAGE_ITEM_MENU_DROP_ALL]);
                add_menu_entry(item_menu, message_list[MESSAGE_CANCEL]);
                menu_auto_set(item_menu);
                item_menu.menu_open = 1;
                found = true;
            }
            if (!found)
            {
                leave = true;
                cursor.x = cursor.y = 0;
            }
            key_lock = true;
        }
        if (p.keyPushed(pb::K_RETURN) && item_menu.menu_open == 1 && !key_lock)
        {
            if (auto* it = selected_item())
                item_type = it->type;
            const std::string entry = item_menu.menu_entry[item_menu.selected_entry];
            const auto& db = item_db[item_type];
            if (entry == message_list[MESSAGE_ITEM_MENU_EQUIP])
            {
                bool can = true;
                if (db.equipment_limit > 0 && number_of_equipped_items(db.class_) >= db.equipment_limit)
                {
                    can = false;
                    std::string t = replace_string(message_list[MESSAGE_EQUIPMENT_LIMIT_REACHED], "[limit]",
                                                   str(db.equipment_limit));
                    t = replace_string(t, "[item_class]", db.class_name);
                    message(t, "", 0xffffff, db.tile, 1);
                }
                if (can)
                {
                    unequip_item_class(item_type);
                    if (auto* it = selected_item())
                        it->equipped = 1;
                }
            }
            else if (entry == message_list[MESSAGE_ITEM_MENU_UNEQUIP])
            {
                if (auto* it = selected_item())
                    it->equipped = 0;
            }
            else if (entry == message_list[MESSAGE_DROP] || entry == message_list[MESSAGE_ITEM_MENU_DROP_ALL])
            {
                bool all = entry == message_list[MESSAGE_ITEM_MENU_DROP_ALL];
                if (auto* it = selected_item())
                {
                    item_on_map_struct m;
                    m.xpos = current_character.xpos;
                    m.ypos = current_character.ypos;
                    m.type = it->type;
                    m.amount = all ? it->amount : 1;
                    m.fuel = it->fuel;
                    m.magic_bonus = it->magic_bonus;
                    item_on_map.push_back(m);
                }
                if (all)
                    character_loses_items(0, 1);
                else
                    character_loses_items(1, 0);
            }
            else if (entry == db.use_name && !db.use_name.empty())
            {
                if (!db.use_message.empty())
                    message(db.use_message, "", 0xffffff, db.tile, 1);
                int value_display = 0;
                for (int i = 0; i < MAX_NUMBER_OF_ITEM_ABILITY_BONI; ++i)
                {
                    int attribute = db.ability_bonus[i].attribute, value = db.ability_bonus[i].value;
                    int duration = db.ability_bonus[i].duration;
                    if (attribute > 0 && value != 0)
                    {
                        if (duration == 0)
                            value_display = modify_attribute(attribute, value);
                        else
                        {
                            auto& a = current_character.ability[attribute];
                            a.power_adjustment = value;
                            a.adjustment_duration = duration;
                            a.adjustment_name = db.name;
                            a.adjustment_item_id = item_type;
                            a.adjustment_power_id = 0;
                        }
                    }
                }
                if (db.power_id > 0)
                    use_power_effect(db.power_id);
                if (db.fuel_consumed_per_use)
                    if (auto* it = selected_item())
                        it->fuel = imax(0, it->fuel - db.fuel_consumed_per_use);
                if (db.consumed_after_use == 1)
                    character_loses_items();
                if (db.turn_ends_after_use == 1)
                    leave = true;
                if (db.identified_when_used == 1)
                    current_character.item_identification[item_type].identified = 1;
                animation(db.use_animation, 6, 6, value_display);
            }
            key_lock = true;
            item_menu.selected_entry = 0;
            item_menu.menu_open = 0;
        }
        if (p.keyReleased(pb::K_ALL))
            key_lock = false;
        blink_cursor(cursor);
        inventory_screen(cursor, item_menu, row_offset);
        p.flip();
    }
    current_character.inventory_cursor_x = cursor.x;
    current_character.inventory_cursor_y = cursor.y;
    update_equipment_boni();
}

// ================================================================ add / remove items
void character_gets_item(int type, int amount, int fuel, int magic_bonus_)
{
    if (fuel == -1)
        fuel = item_db[type].fuel_randomized == 1 ? rnd(imax(1, item_db[type].fuel) - 1) + 1 : item_db[type].fuel;
    if (magic_bonus_ == -1)
        magic_bonus_ = item_db[type].artefact == 1 ? magic_bonus() : 0;
    for (auto& it : inventory)
        if (it.type == type && it.fuel == fuel && it.magic_bonus == magic_bonus_)
        {
            it.amount += 1;
            return;
        }
    item_in_inventory_struct n;
    n.type = type;
    n.amount = amount + 1;  // as in the original
    n.fuel = fuel;
    n.magic_bonus = magic_bonus_;
    inventory.push_back(n);
}

int character_loses_items(int amount_lost, int all)
{
    int rc = 0;
    for (auto it = inventory.begin(); it != inventory.end();)
    {
        if (it->selected == 1)
        {
            rc += all == 1 ? it->amount : imin(amount_lost, it->amount);
            it->amount -= amount_lost;
            if (it->amount < 1 || all == 1)
            {
                it = inventory.erase(it);
                continue;
            }
        }
        ++it;
    }
    return rc;
}

void character_gets_starting_equipment()
{
    inventory.clear();
    for (int i = 1; i < MAX_NUMBER_OF_ITEMS; ++i)
        if (item_db[i].starting_equipment > 0)
            character_gets_item(i, item_db[i].starting_equipment);
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        if (current_character.ability[i].ability_value > 0)
            for (int j = 0; j < MAX_NUMBER_OF_ABILITY_STARTING_EQUIPMENT; ++j)
                if (ability_db[i].starting_equipment[j].item > 0)
                    character_gets_item(ability_db[i].starting_equipment[j].item,
                                        ability_db[i].starting_equipment[j].amount);
}

// ================================================================ pickup
static void pickup_item_screen(cursor_struct& cursor, int xpos, int ypos)
{
    auto& p = P();
    draw_side_panel_frame();
    p.setFont(pb::FONT_DEFAULT);
    p.text(416 + 16 + 2, 0, message_list[MESSAGE_PICKUP_ITEMS] + ":", 0xffffff);
    for (auto& it : item_on_map)
    {
        if (it.selected != 1)
            continue;
        const auto& db = item_db[it.type];
        int y = 240;
        std::string text = db.name;
        if (db.artefact == 1)
            text += " + " + str(it.magic_bonus);
        if (wrap_text(468, y, text, 640 - 468, 16, rgb(0, 255, 255)) < 2)
            p.text(468, (float)(y + 16), db.class_, rgb(255, 100, 0));
        y += 32;
        p.setFont(pb::FONT_VERDANA);
        if (it.amount > 1)
        {
            p.text(434, (float)y, message_list[MESSAGE_AMOUNT] + ": " + str(it.amount), rgb(255, 100, 0));
            y += 16;
        }
        for (int i = 0; i < MAX_NUMBER_OF_ITEM_ABILITY_BONI; ++i)
        {
            const auto& ab = db.ability_bonus[i];
            int bonus = ab.value;
            if (db.artefact == 1)
                bonus += ab.magic_bonus_multiplier * it.magic_bonus;
            if (ab.attribute > 0)
            {
                std::string t = ability_db[ab.attribute].name;
                if (bonus > 0)
                    t += " + ";
                t += str(bonus);
                if (ability_db[ab.attribute].percentage == 1)
                    t += "%";
                p.text(434, (float)y, shorten_text(t, 170), rgb(0, 255, 0));
                y += 16;
            }
        }
        if (db.fuel > 0 && !db.fuel_name.empty())
        {
            p.text(434, (float)y, shorten_text(replace_string(db.fuel_name, "[fuel]", str(it.fuel)), 170), rgb(255, 0, 0));
            y += 16;
        }
        p.text(434, (float)y,
               shorten_text(message_list[MESSAGE_WEIGHT] + ": " + str(db.weight) + " " + message_list[MESSAGE_POUND], 170),
               rgb(255, 0, 0));
        y += 16;
        p.setFont(pb::FONT_SMALL);
        wrap_text(434, y, db.description, 170, 12, 0xffffff);
    }
    for (auto& it : item_on_map)
        it.selected = 0;
    int cell = 0;
    for (auto& it : item_on_map)
    {
        if (it.xpos != xpos || it.ypos != ypos || cell >= 36)
            continue;
        int x = cell % 6, y = cell / 6;
        float px = 434.f + x * 32, py = 32.f + y * 32;
        if (it.selected_for_pickup == 1)
        {
            draw_sheet_tile(SPRITE_ITEMS, TILE_ITEM_EQUIPPED, px, py, false);
            draw_sheet_tile(SPRITE_ITEMS, item_db[it.type].tile, px, py);
        }
        else
            draw_sheet_tile(SPRITE_ITEMS, item_db[it.type].tile, px, py, false);
        if (cursor.x == x && cursor.y == y)
        {
            it.selected = 1;
            draw_sheet_tile(SPRITE_ITEMS, item_db[it.type].tile, 434, 240, false);
            p.blit(sheets[SPRITE_CURSOR], cursor.frame * 32, 0, 32, 32, px, py);
        }
        ++cell;
    }
    for (; cell < 36; ++cell)
    {
        int x = cell % 6, y = cell / 6;
        float px = 434.f + x * 32, py = 32.f + y * 32;
        draw_sheet_tile(SPRITE_POWERS, 0, px, py, false);
        if (cursor.x == x && cursor.y == y)
            p.blit(sheets[SPRITE_CURSOR], cursor.frame * 32, 0, 32, 32, px, py);
    }
}

int pickup_items(int xpos, int ypos)
{
    auto& p = P();
    int rc = 0, number_of_items = 0;
    bool key_lock = true, leave = false, esc = false;
    for (auto& it : item_on_map)
    {
        it.selected = 0;
        it.selected_for_pickup = 0;
        if (it.xpos == xpos && it.ypos == ypos)
            ++number_of_items;
    }
    if (number_of_items == 0)
    {
        leave = true;
        message(message_list[MESSAGE_NO_ITEMS_TO_PICKUP], "");
        play_sound("failure");
    }
    cursor_struct cursor;
    reset_cursor(cursor);
    cursor.x = cursor.y = 0;
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
        if (p.keyPushed(pb::K_LEFT) && !key_lock && cursor.x > 0)
        {
            --cursor.x;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_RIGHT) && !key_lock && cursor.x < 5)
        {
            ++cursor.x;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_UP) && !key_lock && cursor.y > 0)
        {
            --cursor.y;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_DOWN) && !key_lock && cursor.y < 5)
        {
            ++cursor.y;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_SPACE) && !key_lock)
            for (auto& it : item_on_map)
                if (it.selected == 1)
                {
                    it.selected_for_pickup = it.selected_for_pickup == 1 ? 0 : 1;
                    key_lock = true;
                }
        if (p.keyPushed(pb::K_A) && !key_lock)
        {
            for (auto& it : item_on_map)
                if (it.xpos == xpos && it.ypos == ypos)
                    it.selected_for_pickup = it.selected_for_pickup == 1 ? 0 : 1;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_RETURN) && !key_lock)
        {
            leave = true;
            key_lock = true;
        }
        if (p.keyReleased(pb::K_ALL))
            key_lock = false;
        blink_cursor(cursor);
        pickup_item_screen(cursor, xpos, ypos);
        p.flip();
    }
    if (!esc)
    {
        int count = 0, item_tile = 0;
        std::string line2;
        for (auto it = item_on_map.begin(); it != item_on_map.end();)
        {
            if (it->selected_for_pickup == 1)
            {
                ++count;
                item_tile = item_db[it->type].tile;
                if (!line2.empty())
                    line2 += ", ";
                line2 += item_db[it->type].name;
                character_gets_item(it->type, it->amount, it->fuel, it->magic_bonus);
                it = item_on_map.erase(it);
            }
            else
                ++it;
        }
        if (count > 0)
        {
            rc = 1;
            std::string line1 = (count == 1 ? message_list[MESSAGE_PICKUP_AN_ITEM] : message_list[MESSAGE_PICKUP_SOME_ITEMS]) + ":";
            message(line1, line2, 0xffffff, item_tile, 1);
        }
        update_equipment_boni();
        int tile_type = read_map_tile_type(current_map, xpos, ypos);
        if (current_map.tileset.tile_type_db[tile_type].container > 0)
        {
            int left = 0;
            for (auto& it : item_on_map)
                if (it.xpos == xpos && it.ypos == ypos)
                    ++left;
            if (left == 0)
                set_map_tile_type(current_map, xpos, ypos, current_map.background_tile);
        }
    }
    return rc;
}

// ================================================================ random items
int number_of_item_types()
{
    int rc = 0;
    for (int i = 1; i < MAX_NUMBER_OF_ITEMS; ++i)
        if (!item_db[i].name.empty())
            ++rc;
    return rc;
}

int magic_bonus()
{
    int rc = 1;
    while (rnd(1) == 0 && rc < 999)
        ++rc;
    return rc;
}

int random_item_type(int rarity, const std::string& cls, int artefact)
{
    std::vector<int> items;
    for (int attempt = 0; attempt < 100 && items.empty(); ++attempt)
        for (int i = 1; i < MAX_NUMBER_OF_ITEMS; ++i)
        {
            bool add = false;
            if (!item_db[i].name.empty() && rnd(rarity) > item_db[i].price)
            {
                add = true;
                if (!cls.empty() && item_db[i].class_ != cls)
                    add = false;
                if (artefact == 1 && item_db[i].artefact == 0)
                    add = false;
            }
            if (add)
                items.push_back(i);
        }
    return items.empty() ? 0 : items[rnd((int)items.size() - 1)];
}

void add_random_item(int x, int y, int rarity, const std::string& cls)
{
    int type = random_item_type(rarity, cls);
    item_on_map_struct it;
    it.xpos = x;
    it.ypos = y;
    it.type = type;
    it.amount = 1;
    it.fuel = item_db[type].fuel;
    it.magic_bonus = 0;
    if (item_db[type].artefact == 1)
        it.magic_bonus = magic_bonus();
    item_on_map.push_back(it);
}

}  // namespace ll
