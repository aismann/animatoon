// graphics.pb and menu.pb
#include "game/Game.h"

#include <cmath>

namespace ll
{

using pb::str;

static inline int KEY(char c)
{
    return pb::K_A + (c - 'A');
}

// ================================================================ sprites
void load_sprites()
{
    struct S
    {
        int id;
        const char* file;
    };
    static const S list[] = {{SPRITE_Characters, "graphics/characters.png"},
                             {SPRITE_Frame, "graphics/frame.png"},
                             {SPRITE_ABILITIES, "graphics/abilities.png"},
                             {SPRITE_SPLASH, "graphics/splash.png"},
                             {SPRITE_MONSTER, "graphics/monster.png"},
                             {SPRITE_MOUSEPOINTER, "graphics/mousepointer.png"},
                             {SPRITE_BLOOD, "graphics/blood.png"},
                             {SPRITE_POWERS, "graphics/powers.png"},
                             {SPRITE_CURSOR, "graphics/cursor.png"},
                             {SPRITE_STARS, "graphics/starsprites.png"},
                             {SPRITE_MISSILES, "graphics/missiles.png"},
                             {SPRITE_ITEMS, "graphics/items.png"},
                             {SPRITE_FX, "graphics/fx.png"},
                             {SPRITE_SILHOUETTE, "graphics/Silhouette.jpg"},
                             {SPRITE_BACKGROUND, "graphics/maxresdefault.jpg"}};
    for (auto& s : list)
    {
        if (pb::fileSize(res_path(s.file)) < 0)
        {
            if (s.id == SPRITE_BACKGROUND)
            {
                sheets[s.id] = -1;
                continue;
            }
            error_message(std::string("load_sprites(): can't load ") + s.file);
        }
        sheets[s.id] = P().loadSheet(s.file);
    }
    for (int i = 0; i < 7; ++i)
        sheets[SPRITE_LIGHT1 + i] = P().loadSheet("graphics/light" + str(i + 1) + ".png");
    for (int i = 0; i < SPRITE_COUNT; ++i)
        tile_columns[i] = sheets[i] >= 0 ? imax(1, P().sheet(sheets[i]).w / 32) : 1;
}

// draws tile `tile` of a 32x32 tile sheet
void draw_sheet_tile(int sheet, int tile, float x, float y, bool transparent, int alpha)
{
    int cols = tile_columns[sheet];
    int ty = tile / cols;
    int tx = tile - ty * cols;
    P().blit(sheets[sheet], tx * 32, ty * 32, 32, 32, x, y, alpha, !transparent);
}

void draw_map_tile(const map_struct& map, int tile, float x, float y, bool transparent)
{
    int cols = imax(1, map.tileset.columns);
    int ty = tile / cols;
    int tx = tile - ty * cols;
    P().blit(map.tileset.sheet, tx * 32, ty * 32, 32, 32, x, y, 255, !transparent);
}

// ================================================================ text helpers
static size_t utf8_prev(const std::string& s, size_t len)  // length after dropping the last character
{
    if (len == 0)
        return 0;
    size_t i = len - 1;
    while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80)
        --i;
    return i;
}

std::string shorten_text(std::string text, int width, const std::string& append)
{
    if (P().textWidth(text + append) > width)
    {
        while (!text.empty() && P().textWidth(text + append) > width)
            text.resize(utf8_prev(text, text.size()));
        text += append;
    }
    return text;
}

int wrap_text(int x, int y, const std::string& text, int width, int height, uint32_t color)
{
    auto tw = [&](const std::string& s) { return P().textWidth(s); };
    auto mid = [](const std::string& s, size_t start1, size_t len = std::string::npos)
    { return start1 > s.size() ? std::string() : s.substr(start1 - 1, len); };
    auto find1 = [](const std::string& s, size_t from1) -> size_t
    {
        size_t p = s.find(' ', from1 - 1);
        return p == std::string::npos ? 0 : p + 1;
    };
    int rc = 0;
    std::string rest = text;
    size_t pos = 0, pos_old = 0;  // pos_old deliberately persists, as in the original
    int line_y = y;
    while (tw(rest) > width)
    {
        pos = find1(rest, 1);
        while (pos > 0 && tw(mid(rest, 1, pos)) < width)
        {
            pos_old = pos;
            pos = find1(rest, pos_old + 1);
        }
        if (pos == 0)
        {
            if (pos_old == 0)
                P().text((float)x, (float)line_y, rest, color);
            else
            {
                P().text((float)x, (float)line_y, mid(rest, 1, pos_old), color);
                line_y += height;
                ++rc;
                P().text((float)x, (float)line_y, mid(rest, pos_old + 1), color);
            }
            rest.clear();
        }
        else
        {
            if (pos_old == 0)
            {
                P().text((float)x, (float)line_y, mid(rest, 1, pos), color);
                rest = mid(rest, pos + 1);
            }
            else
            {
                P().text((float)x, (float)line_y, mid(rest, 1, pos_old), color);
                rest = mid(rest, pos_old + 1);
            }
        }
        line_y += height;
        ++rc;
    }
    if (!rest.empty())
    {
        P().text((float)x, (float)line_y, rest, color);
        ++rc;
    }
    return rc;
}

void draw_text_centered_number(int x, int y, const std::string& text)
{
    P().setFont(pb::FONT_DEFAULT);
    P().text(x + 16 - P().textWidth(text) / 2, y + 16 - P().textHeight(text) / 2, text, 0xffffff);
}

void screenshot(const std::string&)
{
    // the original grabbed a JPEG that is never read back; nothing to do in the port
}

// ================================================================ frames
void draw_frame_main_screen()
{
    auto& p = P();
    int sh = sheets[SPRITE_Frame];
    int frame = 1;
    p.blit(sh, 0, 6 * 16, 16, 16, 0, 26 * 16);
    for (int i = 1; i <= 39; ++i)
    {
        p.blit(sh, 0, 7 * 16 + frame * 16, 16, 16, i * 16.f, 26 * 16.f, 255, true);
        if (i > 25)
        {
            p.blit(sh, 0, 7 * 16 + frame * 16, 16, 16, i * 16.f, 18, 255, true);
            p.blit(sh, 0, 7 * 16 + frame * 16, 16, 16, i * 16.f, 66, 255, true);
        }
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

void draw_standard_frame(const std::string& title, const std::string& subtitle)
{
    auto& p = P();
    int sh = sheets[SPRITE_Frame];
    int frame = 1;
    p.clearScreen(0);
    for (int i = 0; i <= 29; ++i)
    {
        p.blit(sh, 0, frame * 16 + 16, 16, 16, 0, i * 16.f, 255, true);
        p.blit(sh, 0, frame * 16 + 16, 16, 16, 624, i * 16.f, 255, true);
        if (++frame > 4)
            frame = 0;
    }
    p.setFont(pb::FONT_DEFAULT);
    for (int i = 1; i <= 38; ++i)
    {
        p.blit(sh, 0, 7 * 16 + frame * 16, 16, 16, i * 16.f, 0, 255, true);
        p.blit(sh, 0, 7 * 16 + frame * 16, 16, 16, i * 16.f, 32, 255, true);
        if (!subtitle.empty())
            p.blit(sh, 0, 7 * 16 + frame * 16, 16, 16, i * 16.f, 432, 255, true);
        p.blit(sh, 0, 7 * 16 + frame * 16, 16, 16, i * 16.f, 464, 255, true);
        if (++frame > 4)
            frame = 0;
    }
    p.text(320 - p.textWidth(title) / 2, 16, title, rgb(255, 255, 255));
    if (!subtitle.empty())
        p.text(320 - p.textWidth(subtitle) / 2, 448, subtitle, rgb(255, 255, 255));
}

// ================================================================ main screen
void message(const std::string& line1, const std::string& line2, uint32_t color, int tile, int tile_type,
             int display_for)
{
    message_.color = color;
    message_.line1 = line1;
    message_.line2 = line2;
    message_.tile = tile;
    message_.tile_type = tile_type;
    message_.start_time = P().elapsedMs();
    message_.display_for = (uint32_t)display_for;
}

void draw_map_screen(map_struct& map, int x_offset, int y_offset, int fade)
{
    auto& p = P();
    for (int y = -1; y <= 12; ++y)
        for (int x = -1; x <= 12; ++x)
        {
            int wx = current_character.xpos - 6 + x, wy = current_character.ypos - 6 + y;
            int tile_type = read_map_tile_type(map, wx, wy);
            if (read_map_hidden(map, wx, wy) == 1)
                tile_type = map.background_tile;
            float px = (float)(x * 32 + x_offset), py = (float)(y * 32 + y_offset);
            if (map.tileset.tile_type_db[tile_type].transparent == 1)
            {
                draw_map_tile(map, current_map.background_tile, px, py, false);
                draw_map_tile(map, tile_type, px, py, true);
            }
            else
                draw_map_tile(map, tile_type, px, py, false);
        }
    // items
    for (auto& it : item_on_map)
        if (it.xpos >= current_character.xpos - 7 && it.xpos <= current_character.xpos + 6 &&
            it.ypos >= current_character.ypos - 7 && it.ypos <= current_character.ypos + 6)
        {
            int tile_type = read_map_tile_type(map, it.xpos, it.ypos);
            if (map.tileset.tile_type_db[tile_type].container == 0)
                draw_sheet_tile(SPRITE_ITEMS, item_db[it.type].tile,
                                (float)((it.xpos - current_character.xpos + 6) * 32 + x_offset),
                                (float)((it.ypos - current_character.ypos + 6) * 32 + y_offset));
        }
    // monsters
    for (auto& mo : monster)
        if (mo.xpos >= current_character.xpos - 7 && mo.xpos <= current_character.xpos + 6 &&
            mo.ypos >= current_character.ypos - 7 && mo.ypos <= current_character.ypos + 6)
            draw_sheet_tile(SPRITE_MONSTER, monster_type_db[mo.type].tile,
                            (float)((mo.xpos - current_character.xpos + 6) * 32 + x_offset + mo.x_offset),
                            (float)((mo.ypos - current_character.ypos + 6) * 32 + y_offset + mo.y_offset));
    int lighting = current_light_radius();
    if (lighting >= 0 && lighting < 7)
        p.blitFull(sheets[SPRITE_LIGHT1 + lighting], 0, 0);
    if (fade > 0 && preferences.fade_effect == 1)
        p.box(0, 0, 416, 416, rgb(1, 1, 1), fade);

    p.box(0, 416, 640, 3 * 16, 0);
    p.box(416, 0, 640 - 416, 480, 0);
    // messages
    bool display_message = false;
    if (message_.display_for > P().elapsedMs() - message_.start_time)
    {
        int x = 36;
        if (message_.tile < 0)
            x = 0;
        p.setFont(pb::FONT_DEFAULT);
        if (!message_.line1.empty() && !message_.line2.empty())
        {
            p.text((float)x, 440, message_.line1, message_.color);
            p.text((float)x, 456, message_.line2, message_.color);
            display_message = true;
        }
        else if (!message_.line1.empty())
        {
            wrap_text(x, 440, message_.line1, 640 - x, 16, message_.color);
            display_message = true;
        }
        else if (!message_.line2.empty())
        {
            p.text((float)x, 448, message_.line2, message_.color);
            display_message = true;
        }
    }
    if (display_message && message_.tile >= 0)
    {
        switch (message_.tile_type)
        {
        case 0: draw_map_tile(map, message_.tile, 0, 440, false); break;
        case 1: draw_sheet_tile(SPRITE_ITEMS, message_.tile, 0, 440, false); break;
        case 2: draw_sheet_tile(SPRITE_POWERS, message_.tile, 0, 440, false); break;
        case 3: draw_sheet_tile(SPRITE_MONSTER, message_.tile, 0, 440, false); break;
        case 4: draw_sheet_tile(SPRITE_ABILITIES, message_.tile, 0, 440, false); break;
        }
    }
}

void draw_character(int y_offset)
{
    int facing = current_character.facing & 3, frame = current_character.frame & 1;
    P().blit(sheets[SPRITE_Characters], facing * 64 + frame * 32, 0, 32, 32, 6 * 32.f, 6 * 32.f + y_offset);
}

void current_character_change_frame()
{
    if (++current_character.frame > 1)
        current_character.frame = 0;
}

void draw_right_panel()
{
    auto& p = P();
    struct T
    {
        int x, y, tile, type;
    };
    std::vector<T> tiles;
    p.setFont(pb::FONT_VERDANA);
    p.text(416 + 16 + 2, 0, program_title, rgb(255, 100, 0));
    int y = 86;
    wrap_text(434, 34, "Location: " + current_map.name, 640 - 434, 16, 0xffffff);
    p.setFont(pb::FONT_SMALL);
    for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
        for (int j = 0; j < MAX_NUMBER_OF_ABILITY_DISPLAYS; ++j)
        {
            const auto& dsp = ability_db[i].display[j];
            if (dsp.type != 1)
                continue;
            int cur = attribute_current_val(i);
            if ((cur >= dsp.start_value && cur <= dsp.end_value) || (dsp.start_value == 0 && dsp.end_value == 0))
            {
                std::string value = str(cur);
                if (dsp.expanded_value_display == 1)
                    value = ability_expanded_value_display(i);
                p.text(468, (float)y, replace_string(dsp.message, "[value]", value, true), dsp.color);
                tiles.push_back({434, y, ability_db[i].tile, 1});
                y += 12;
                if (!dsp.message2.empty())
                    p.text(468, (float)y, replace_string(dsp.message2, "[value]", value, true), dsp.color);
                y += 12 + 10;
            }
        }
    for (int i = 0; i < MAX_NUMBER_OF_ABILITIES; ++i)
    {
        const auto& ab = current_character.ability[i];
        if (!ab.adjustment_name.empty())
        {
            T t{434, y, 0, 0};
            if (ab.adjustment_power_id > 0)
            {
                t.tile = power_db[ab.adjustment_power_id].tile;
                t.type = 2;
            }
            if (ab.adjustment_item_id > 0)
            {
                t.tile = item_db[ab.adjustment_item_id].tile;
                t.type = 3;
            }
            tiles.push_back(t);
            p.text(434 + 34, (float)y, ab.adjustment_name, 0xffffff);
            y += 12;
            p.text(434 + 34, (float)y,
                   message_list[MESSAGE_STRENGTH] + ": " + str(ab.power_adjustment) + " " +
                       message_list[MESSAGE_DURATION] + ": " + str(ab.adjustment_duration),
                   0xffffff);
            y += 22;
        }
        if (ab.activated == 1)
        {
            tiles.push_back({434, y, ability_db[i].tile, 1});
            p.text(434 + 34, (float)y, ability_db[i].name_display, 0xffffff);
            y += 12;
            p.text(434 + 34, (float)y, ability_expanded_value_display(i), 0xffffff);
            y += 22;
        }
    }
    for (auto& t : tiles)
    {
        if (t.type == 1)
            draw_sheet_tile(SPRITE_ABILITIES, t.tile, (float)t.x, (float)t.y, false);
        else if (t.type == 2)
            draw_sheet_tile(SPRITE_POWERS, t.tile, (float)t.x, (float)t.y, false);
        else if (t.type == 3)
            draw_sheet_tile(SPRITE_ITEMS, t.tile, (float)t.x, (float)t.y, false);
    }
}

void draw_game_scene()
{
    P().clearScreen(0);
    draw_map_screen(current_map);
    draw_frame_main_screen();
    draw_right_panel();
    draw_character();
}

void draw_main_screen_complete(map_struct& map, int x_offset, int y_offset, int fade, int char_y_offset)
{
    P().clearScreen(0);
    draw_map_screen(map, x_offset, y_offset, fade);
    draw_frame_main_screen();
    draw_right_panel();
    draw_character(char_y_offset);
    P().flip();
}

void fade_map_screen_out(map_struct& map)
{
    for (int i = 1; i <= 255; i += 5)
        draw_main_screen_complete(map, 0, 0, i);
}

void fade_map_screen_in(map_struct& map)
{
    for (int i = 255; i >= 0; i -= 5)
        draw_main_screen_complete(map, 0, 0, i);
}

// ================================================================ cursor
void blink_cursor(cursor_struct& c)
{
    if (c.dir == 1)
    {
        if (++c.frame > 3)
            c.dir = -1;
    }
    else
    {
        if (--c.frame < 1)
            c.dir = 1;
    }
}

void reset_cursor(cursor_struct& c)
{
    c = cursor_struct{};
}

void cursor_select_field(cursor_struct& c, int range)
{
    auto& p = P();
    bool leave = false, key_lock = true;
    while (!leave)
    {
        p.examineKeyboard();
        if (p.keyPushed(pb::K_UP) && !key_lock)
            if ((range > 1 && c.y > 0) || (range == 1 && c.y > 5))
            {
                --c.y;
                key_lock = true;
            }
        if (p.keyPushed(pb::K_DOWN) && !key_lock)
            if ((range > 1 && c.y < 12) || (range == 1 && c.y < 7))
            {
                ++c.y;
                key_lock = true;
            }
        if (p.keyPushed(pb::K_LEFT) && !key_lock)
            if ((range > 1 && c.x > 0) || (range == 1 && c.x > 5))
            {
                --c.x;
                key_lock = true;
            }
        if (p.keyPushed(pb::K_RIGHT) && !key_lock)
            if ((range > 1 && c.x < 12) || (range == 1 && c.x < 7))
            {
                ++c.x;
                key_lock = true;
            }
        if (p.keyReleased(pb::K_RETURN) && !key_lock)
            leave = true;
        if (p.keyReleased(pb::K_ESCAPE))
        {
            leave = true;
            c.esc = 1;
        }
        if (p.keyReleased(pb::K_ALL))
            key_lock = false;
        draw_game_scene();
        p.blit(sheets[SPRITE_CURSOR], c.frame * 32, 0, 32, 32, c.x * 32.f, c.y * 32.f);
        blink_cursor(c);
        p.flip();
    }
}

// ================================================================ animations
void star_sprite_animation(int star_sprite, int x, int y, int number_of_stars, int mixed, int second_star_sprite)
{
    std::vector<star_sprites_struct> stars;
    auto pick_color = [&](star_sprites_struct& s)
    {
        s.color = star_sprite;
        if (mixed == 1)
            s.color = rnd(4);
        else if (second_star_sprite > 0 && rnd(1) == 0)
            s.color = second_star_sprite;
    };
    for (int i = 1; i <= number_of_stars; ++i)
    {
        star_sprites_struct s;
        s.x = x * 32 + rnd(30);
        s.y = y * 32 + rnd(30);
        s.frame = 0;
        s.delay = rnd(9);
        s.dir = 1;
        pick_color(s);
        stars.push_back(s);
    }
    for (int count = 0; count < 50; ++count)
    {
        draw_game_scene();
        for (auto& s : stars)
        {
            if (s.delay > 0)
            {
                --s.delay;
                continue;
            }
            P().blit(sheets[SPRITE_STARS], s.frame * 9, s.color * 9, 9, 9, (float)s.x, (float)s.y);
            if (s.dir == 1)
            {
                if (++s.frame > 3)
                {
                    s.frame = 4;
                    s.dir = -1;
                }
            }
            else if (--s.frame < 1)
            {
                s.frame = 0;
                s.x = x * 32 + rnd(30);
                s.y = y * 32 + rnd(30);
                s.dir = 1;
                pick_color(s);
            }
        }
        P().flip();
        P().delayMs(25);
    }
}

void star_circle_animation(int star_sprite, int x, int y)
{
    for (int i = 0; i <= 200; i += 5)
    {
        draw_game_scene();
        for (int a = 0; a <= 360; a += 20)
            P().blit(sheets[SPRITE_STARS], 4 * 9, star_sprite * 9, 9, 9, x * 32 + 12 + std::sin((double)(a + i)) * 20,
                     y * 32 + 12 + std::cos((double)(a + i)) * 20);
        P().flip();
        P().delayMs(25);
    }
}

void missile_animation(int tile, int source_x, int source_y, int destination_x, int destination_y)
{
    int x = source_x * 32, y = source_y * 32;
    int step_x = destination_x - source_x, step_y = destination_y - source_y;
    int count = 32;
    if (std::abs(step_x) > std::abs(step_y) && step_y != 0)
        count = (std::abs(step_x) / std::abs(step_y)) * 32;
    if (std::abs(step_y) > std::abs(step_x) && step_x != 0)
        count = (std::abs(step_y) / std::abs(step_x)) * 32;
    for (int i = 0; i <= count; ++i)
    {
        draw_game_scene();
        draw_sheet_tile(SPRITE_MISSILES, tile, (float)x, (float)y);
        P().flip();
        x += step_x;
        y += step_y;
    }
}

void display_tile_animation(int tile, int type, int x, int y, int display_number, int number)
{
    if (type == 1)
        draw_map_tile(current_map, tile, x * 32.f, y * 32.f, true);
    else
        draw_sheet_tile(SPRITE_FX, tile, x * 32.f, y * 32.f);
    if (display_number == 1)
        draw_text_centered_number(x * 32, y * 32, str(number));
    P().flip();
    P().delayMs(250);
    draw_game_scene();
    P().flip();
    P().delayMs(250);
}

void display_tile_flood_animation(int tile)
{
    P().clearScreen(0);
    draw_map_screen(current_map);
    for (int x = 0; x <= 12; ++x)
        for (int y = 0; y <= 12; ++y)
            draw_sheet_tile(SPRITE_FX, tile, x * 32.f, y * 32.f);
    draw_frame_main_screen();
    draw_right_panel();
    draw_character();
    P().flip();
    P().delayMs(500);
    draw_game_scene();
    P().flip();
}

void animation(const std::string& name, int x, int y, int param)
{
    if (name.empty())
        return;
    static const char* colors[] = {"blue", "red", "purple", "green", "yellow", "aqua", "grey", "skull"};
    for (int i = 0; i < 8; ++i)
    {
        if (name == std::string(colors[i]) + " stars")
            return star_sprite_animation(i, x, y, 25);
        if (name == std::string(colors[i]) + " circle")
            return star_circle_animation(i, x, y);
    }
    if (name == "mixed stars")
        return star_sprite_animation(0, x, y, 25, 1);
    if (name == "red fireball")
        return missile_animation(SPRITE_MISSILE_RED_BALL, 6, 6, x, y);
    if (name == "blue fireball")
        return missile_animation(SPRITE_MISSILE_BLUE_BALL, 6, 6, x, y);
    if (name == "green fireball")
        return missile_animation(SPRITE_MISSILE_GREEN_BALL, 6, 6, x, y);
    if (name == "purple fireball")
        return missile_animation(SPRITE_MISSILE_PURPLE_BALL, 6, 6, x, y);
    if (name == "lightning")
        return missile_animation(SPRITE_MISSILE_LIGHTNING, 6, 6, x, y);
    if (name == "blue lightning")
        return missile_animation(SPRITE_MISSILE_BLUE_LIGHTNING, 6, 6, x, y);
    if (name == "red cloud")
        return display_tile_animation(SPRITE_FX_CLOUD_RED, 0, x, y, 1, param);
    if (name == "green cloud")
        return display_tile_animation(SPRITE_FX_CLOUD_GREEN, 0, x, y, 1, param);
    if (name == "magic shield")
        return display_tile_animation(SPRITE_FX_MAGIC_SHIELD, 0, x, y, 1, param);
    if (name == "battle rage")
        return display_tile_animation(SPRITE_FX_RAGE, 0, x, y, 1, param);
    if (name == "fire flood")
        return display_tile_flood_animation(SPRITE_FX_FIRE);
    if (name == "wind flood")
        return display_tile_flood_animation(SPRITE_FX_WIND);
    if (name == "cross flood")
        return display_tile_flood_animation(SPRITE_FX_CROSS);
    error_message("animation(): unknown animation: \"" + name + "\"");
}

// ================================================================ menu (menu.pb)
void reset_menu(menu_struct& m)
{
    m = menu_struct{};
    m.menu_entry.assign(30, "");
}

void menu_reset_entries(menu_struct& m)
{
    for (auto& e : m.menu_entry)
        e.clear();
    m.number_of_entries = 0;
}

int add_menu_entry(menu_struct& m, const std::string& option)
{
    for (size_t i = 0; i < m.menu_entry.size(); ++i)
        if (m.menu_entry[i].empty())
        {
            m.menu_entry[i] = option;
            return (int)i;
        }
    return 0;
}

void menu_auto_set(menu_struct& m)
{
    m.number_of_entries = 0;
    P().setFont(m.font);
    for (auto& e : m.menu_entry)
    {
        int w = (int)P().textWidth(e) + m.padding + m.padding;
        if (w > m.width)
            m.width = w;
        int h = (int)P().textHeight(e) + m.padding + m.padding;
        if (h > m.entry_height)
            m.entry_height = h;
        if (!e.empty())
            ++m.number_of_entries;
    }
}

void display_menu(menu_struct& m)
{
    if (m.menu_open != 1)
        return;
    auto& p = P();
    p.setFont(m.font);
    int x = m.x, y = m.y;
    if (m.width == 0)
        menu_auto_set(m);
    m.mouse_over_entry = -1;
    if (m.border > 0)
    {
        p.box(x - 1.f, y - 1.f, m.width + 3.f, (m.entry_height + m.padding + m.padding + m.spacing) * m.number_of_entries + 3.f,
              m.color);
        p.box((float)x, (float)y, (float)m.width, (float)((m.entry_height + m.spacing) * m.number_of_entries), 0);
    }
    for (int i = 0; i < m.number_of_entries; ++i)
    {
        if (m.entry_border > 0 || m.selected_entry == i)
        {
            p.box((float)x, (float)y, (float)m.width, (float)m.entry_height, m.color);
            if (m.selected_entry == i && m.entry_highlight == 1)
            {
                p.line((float)x, (float)y, (float)(x + m.width), (float)y, m.entry_highlight_color1);
                p.line((float)x, (float)y, (float)x, (float)(y + m.entry_height), m.entry_highlight_color1);
                p.line((float)(x + m.width), y + 1.f, (float)(x + m.width), (float)(y + m.entry_height),
                       m.entry_highlight_color2);
                p.line(x + 1.f, (float)(y + m.entry_height), (float)(x + m.width), (float)(y + m.entry_height),
                       m.entry_highlight_color2);
            }
        }
        if (m.use_mouse == 1)
            if (m.mouse_x >= m.x && m.mouse_x <= m.x + m.width && m.mouse_y >= y && m.mouse_y <= y + m.entry_height &&
                m.mouse_over_entry == -1)
            {
                m.selected_entry = i;
                m.mouse_over_entry = i;
            }
        float tx = (float)(x + m.padding + m.entry_border + m.border);
        float ty = (float)(y + m.padding + m.entry_border + m.border);
        if (m.selected_entry == i)
        {
            // black text on the coloured entry (fixed colour: 0 means "transparent" in this port)
            p.text(tx, ty, m.menu_entry[i], rgb(0, 0, 1));
        }
        else
        {
            p.box(x + 1.f, y + 1.f, (float)(m.width - m.entry_border * 2), m.entry_height - 2.f, 0);
            p.text(tx, ty, m.menu_entry[i], m.color);
        }
        y += m.entry_height + m.spacing + m.padding;
    }
}

std::string main_menu()
{
    auto& p = P();
    static menu_struct menu;
    reset_menu(menu);
    if (pb::fileSize(save_path("character.xml")) > 0)
        add_menu_entry(menu, message_list[MESSAGE_MENU_SPLASH_CONTINUE_GAME]);
    add_menu_entry(menu, message_list[MESSAGE_MENU_SPLASH_NEW_GAME]);
    add_menu_entry(menu, message_list[MESSAGE_MENU_SPLASH_EXIT]);
    menu.color = rgb(255, 100, 0);
    menu.padding = 6;
    menu.x = 280;
    menu.y = 250;
    menu.use_mouse = 1;
    menu.entry_highlight = 1;
    menu.entry_highlight_color1 = rgb(255, 200, 125);
    menu.entry_highlight_color2 = rgb(125, 75, 0);

    bool leave = false, key_lock = false;
    bool bg = sheets[SPRITE_BACKGROUND] >= 0;
    auto draw = [&]()
    {
        p.clearScreen(0);
        if (bg)
            p.blitFull(sheets[SPRITE_BACKGROUND], 0, 0, 255, true);
        int sw = p.sheet(sheets[SPRITE_SPLASH]).w;
        p.blitFull(sheets[SPRITE_SPLASH], 320.f - sw / 2, 0);
        display_menu(menu);
        if (!current_character.game_end_message.empty())
        {
            p.setFont(pb::FONT_DEFAULT);
            p.text(320 - p.textWidth(current_character.game_end_message) / 2, 170, current_character.game_end_message,
                   rgb(255, 0, 0));
        }
    };
    draw();
    p.flip();
    p.takeClick();
    while (!leave)
    {
        p.examineKeyboard();
        menu.mouse_x = p.mouseX();
        menu.mouse_y = p.mouseY();
        if (p.takeClick() && menu.mouse_over_entry > -1)
            leave = true;
        if (p.keyReleased(pb::K_ESCAPE))
        {
            program_ends = 1;
            leave = true;
        }
        if (p.keyPushed(pb::K_UP) && !key_lock)
        {
            if (--menu.selected_entry < 0)
                menu.selected_entry = menu.number_of_entries - 1;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_DOWN) && !key_lock)
        {
            if (++menu.selected_entry >= menu.number_of_entries)
                menu.selected_entry = 0;
            key_lock = true;
        }
        if (p.keyPushed(pb::K_RETURN) && !key_lock)
            leave = true;
        if (p.keyReleased(pb::K_ALL))
            key_lock = false;
        draw();
        p.flip();
    }
    std::string chosen = menu.menu_entry[menu.selected_entry];
    if (chosen == message_list[MESSAGE_MENU_SPLASH_NEW_GAME])
        delete_savegame();
    else if (chosen == message_list[MESSAGE_MENU_SPLASH_EXIT])
        program_ends = 1;
    return chosen;
}

}  // namespace ll
