// main.pb - the main game loop (runs on the game thread, uses blocking calls like the original)
#include "game/Game.h"

namespace ll
{

static inline int KEY(char c)
{
    return pb::K_A + (c - 'A');
}

static void init_resources()
{
    load_preferences();
    load_message_list();
    load_sound();
    load_sprites();
    load_abilities_db();
    load_monster_db();
    load_power_db();
    load_item_db();
    ability_db_resolve_names();
    item_db_resolve_names();
}

static void leave_to_menu_screen()
{
    // draws the paused game once more under a fade (the screenshot is not needed in the port)
    draw_game_scene();
    if (preferences.fade_effect == 1)
        P().box(0, 0, 640, 480, rgb(1, 1, 1), 150);
    P().flip();
}

void game_main()
{
    auto& p = P();
    pb::makeDirs(P().writeRoot() + "/savegame");
    pb::makeDirs(P().writeRoot() + "/config");
    init_resources();

    if (pb::fileSize(save_path("character.xml")) > 0)
    {
        load_character(current_character, save_path("character.xml"));
        load_map(current_map, save_path(file_part(current_character.map_filename)));
    }

    int update_screen = 1, game_paused = 0, turn_ends = 0, goto_main_menu = 1, character_died_flag = 0;
    int key_lock = 0, update_equipment_boni_flag = 0, blocked_by_monster;
    int dx = 0, dy = 0;
    int character_moved = 0;

    try
    {
        while (program_ends == 0)
        {
            p.examineKeyboard();
            character_moved = 0;
            dx = dy = 0;

            // ---- main menu
            if (goto_main_menu == 1)
            {
                update_screen = 1;
                goto_main_menu = 0;
                game_paused = 0;
                program_ends = 0;
                character_died_flag = 0;
                std::string choice = main_menu();
                if (choice == message_list[MESSAGE_MENU_SPLASH_NEW_GAME])
                {
                    if (program_ends == 0)
                    {
                        delete_savegame();
                        init_character(current_character);
                        experience(3, 1);
                        character_gets_starting_equipment();
                        load_map(current_map, res_path("maps/" + file_part(current_character.map_filename)));
                        save_character(current_character, save_path("character.xml"));
                        save_map(current_map, save_path(file_part(current_map.filename)));
                    }
                }
                else if (choice == message_list[MESSAGE_MENU_SPLASH_CONTINUE_GAME])
                {
                    if (program_ends == 0)
                    {
                        load_character(current_character, save_path("character.xml"));
                        load_map(current_map, save_path(file_part(current_character.map_filename)));
                    }
                }
                else if (choice == message_list[MESSAGE_MENU_SPLASH_EXIT])
                    program_ends = 1;
                if (program_ends)
                    break;
            }

            // ---- escape: back to the main menu
            if (p.keyReleased(pb::K_ESCAPE))
            {
                leave_to_menu_screen();
                save_map(current_map, save_path(file_part(current_map.filename)));
                save_character(current_character, save_path("character.xml"));
                goto_main_menu = 1;
                game_paused = 1;
            }

            // ---- ALT-X ends the program without saving
            if (p.keyPushed(pb::K_LEFT_ALT) && p.keyPushed(KEY('X')))
                program_ends = 1;

            // ---- movement keys
            auto steer = [&](int action, int ddx, int ddy, int facing)
            {
                if (p.keyPushed(key_binding[action].keycode) && game_paused == 0)
                {
                    if (movement_left() > 0)
                    {
                        dx = ddx;
                        dy = ddy;
                    }
                    current_character.facing = facing;
                    update_screen = 1;
                }
            };
            steer(ACTION_UP, 0, -1, 0);
            steer(ACTION_DOWN, 0, 1, 2);
            steer(ACTION_RIGHT, 1, 0, 1);
            steer(ACTION_LEFT, -1, 0, 3);

            if ((dx != 0 || dy != 0) && game_paused == 0)
            {
                update_screen = 1;
                blocked_by_monster = 0;
                int nx = current_character.xpos + dx, ny = current_character.ypos + dy;
                if (nx >= 0 && nx < MAP_DIMENSION_X && ny >= 0 && ny < MAP_DIMENSION_Y)
                {
                    if (field_contains_monster(nx, ny))
                    {
                        blocked_by_monster = 1;
                        if (sneaking_val(1) > 0 && backstabbing_val(1) == 0)
                        {
                            if (rnd(99) >= sneaking_val(1) || monster_is_alerted(nx, ny))
                            {
                                play_sound("oh_no");
                                alert_monster(nx, ny);
                                message(message_list[MESSAGE_MONSTER_NOTICES_YOU], "");
                                turn_ends = 1;
                            }
                            else
                            {
                                blocked_by_monster = 0;
                                play_sound("evasion");
                                message(message_list[MESSAGE_YOU_SNEAK_UNNOTICED_BY], "");
                            }
                        }
                        else
                        {
                            character_attacks_monster(current_map, nx, ny);
                            turn_ends = 1;
                        }
                    }
                    if (blocked_by_monster == 0)
                    {
                        trigger_map_event(current_map, nx, ny);
                        trigger_map_message(current_map, nx, ny);
                        if (field_is_blocked(current_map, nx, ny) == 0)
                        {
                            current_character_change_frame();
                            if (dx > 0 || dy > 0)
                            {
                                current_character.xpos += dx;
                                current_character.ypos += dy;
                                for (int i = 30; i >= 2; i -= SETTING_MAP_SCREEN_SCROLLING_STEP)
                                {
                                    draw_main_screen_complete(current_map, dx * i, dy * i);
                                    current_character_change_frame();
                                }
                            }
                            else
                            {
                                for (int i = 2; i <= 30; i += SETTING_MAP_SCREEN_SCROLLING_STEP)
                                {
                                    draw_main_screen_complete(current_map, std::abs(dx) * i, std::abs(dy) * i);
                                    current_character_change_frame();
                                }
                                current_character.xpos += dx;
                                current_character.ypos += dy;
                            }
                            character_moved = 1;
                        }
                    }
                }
            }

            // ---- character info
            if (p.keyPushed(KEY('C')) && game_paused == 0)
            {
                character_info_screen();
                p.flip();
                game_paused = 1;
            }
            if (p.keyPushed(KEY('U')) && game_paused == 0 && key_lock == 0)
            {
                if (use_power() == 1)
                    turn_ends = 1;
                update_screen = 1;
                key_lock = 1;
            }
            if (p.keyPushed(KEY('P')) && game_paused == 0 && key_lock == 0)
            {
                pickup_items(current_character.xpos, current_character.ypos);
                key_lock = 1;
                update_screen = 1;
            }
            if (p.keyPushed(KEY('I')) && game_paused == 0 && key_lock == 0)
            {
                open_inventory();
                update_screen = 1;
                key_lock = 1;
            }
            if (p.keyPushed(KEY('E')) && game_paused == 0 && key_lock == 0)
            {
                turn_ends = 1;
                update_screen = 1;
                key_lock = 1;
            }
            if (p.keyPushed(KEY('T')) && key_lock == 0)
                scramble_item_db_tiles("Potion");

            // ---- field effects after the character has moved
            if (character_moved == 1 && game_paused == 0)
            {
                if (enter_field() == 1)
                    turn_ends = 1;
                if (detect_hidden_fields() == 1)
                {
                    draw_main_screen_complete(current_map, 0, 0);
                    p.delayMs(1000);
                }
                if (field_is_open_portal(current_map, current_character.xpos, current_character.ypos) == 1)
                {
                    enter_portal();
                    update_screen = 1;
                }
                else if (field_contains_dungeon_entrance(current_character.xpos, current_character.ypos))
                {
                    enter_dungeon();
                    update_screen = 1;
                }
                for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
                {
                    if (ability_db[i].change_per_move != 0)
                        modify_attribute(i, ability_db[i].change_per_move);
                    if (turn_ends == 0 && ability_db[i].turn_ends_if_lower > -32000)
                        if (attribute_current_val(i) < ability_db[i].turn_ends_if_lower)
                            turn_ends = 1;
                }
            }

            // ---- leave paused status
            if (p.keyPushed(pb::K_RETURN) && game_paused == 1)
            {
                game_paused = 0;
                update_screen = 1;
            }
            if (p.keyReleased(pb::K_ALL))
                key_lock = 0;

            // ---- end of turn
            if (turn_ends == 1)
            {
                for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
                {
                    const auto& db = ability_db[i];
                    if (db.modify_attribute > 0)
                    {
                        int mv = attribute_current_val(i);
                        if (mv != 0)
                        {
                            if (db.modify_divisor > 1)
                                mv = (mv + db.modify_divisor - 1) / db.modify_divisor;
                            if (db.modify_multiplier != 1)
                                mv *= db.modify_multiplier;
                        }
                        if (mv != 0)
                        {
                            modify_attribute(db.modify_attribute, mv);
                            if (!db.modify_sound.empty())
                                play_sound(db.modify_sound);
                            if (!db.modify_animation.empty())
                                animation(db.modify_animation, 6, 6, std::abs(mv));
                        }
                    }
                    if (db.change_per_turn != 0 && rnd(99) < db.change_per_turn_chance)
                        modify_attribute(i, db.change_per_turn);
                    auto& ca = current_character.ability[i];
                    if (ca.power_adjustment != 0 || ca.adjustment_duration != 0)
                    {
                        if (--ca.adjustment_duration < 1)
                        {
                            ca.power_adjustment = 0;
                            ca.adjustment_duration = 0;
                            ca.adjustment_name.clear();
                            ca.adjustment_power_id = 0;
                        }
                    }
                }
                for (int i = 1; i < MAX_NUMBER_OF_POWERS; ++i)
                    if (current_character.power[i].cooldown > 0)
                        current_character.power[i].cooldown = imax(0, current_character.power[i].cooldown - 1);
                move_all_monster();
                monster_attacks_character(current_character.xpos, current_character.ypos - 1);
                monster_attacks_character(current_character.xpos + 1, current_character.ypos);
                monster_attacks_character(current_character.xpos, current_character.ypos + 1);
                monster_attacks_character(current_character.xpos - 1, current_character.ypos);
                monster_turn_ends();
                spawn_random_monster(current_map);
                for (auto& it : inventory)
                    if (it.equipped == 1 && item_db[it.type].fuel > 0)
                    {
                        it.fuel -= item_db[it.type].fuel_consumed_per_turn;
                        if (it.fuel < 1)
                        {
                            it.fuel = 0;
                            it.equipped = 0;
                            update_equipment_boni_flag = 1;
                        }
                    }
                for (int i = 1; i < MAX_NUMBER_OF_ABILITIES; ++i)
                    if (ability_db[i].restore_at_end_of_turn == 1)
                        restore_attribute(i);
                turn_ends = 0;
                update_screen = 1;
            }

            // ---- death
            if (character_died() && program_ends == 0)
            {
                play_sound("deathcry");
                fade_map_screen_out(current_map);
                draw_game_scene();
                if (preferences.fade_effect == 1)
                    p.box(0, 0, 640, 480, rgb(1, 1, 1), 150);
                p.flip();
                if (startLastPoint == 0)
                    delete_savegame();
                character_died_flag = 1;
                goto_main_menu = 1;
                game_paused = 1;
                update_screen = 0;
            }

            if (update_equipment_boni_flag == 1)
            {
                update_equipment_boni();
                update_equipment_boni_flag = 0;
            }

            if (update_screen == 1)
            {
                draw_main_screen_complete(current_map, 0, 0);
                update_screen = 0;
            }
            else
                p.delayMs(8);  // nothing drawn this round: don't spin
        }

        if (pb::fileSize(save_path("character.xml")) > 0 && pb::fileSize(save_path(file_part(current_map.filename))) >= 0 &&
            !current_map.filename.empty())
        {
            save_map(current_map, save_path(file_part(current_map.filename)));
            save_character(current_character, save_path("character.xml"));
        }
    }
    catch (const pb::QuitException&)
    {
        // window closed: same behaviour as the original (save and leave)
        if (!current_map.filename.empty() && pb::fileSize(save_path("character.xml")) > 0 && !character_died_flag)
        {
            try
            {
                save_map(current_map, save_path(file_part(current_map.filename)));
                save_character(current_character, save_path("character.xml"));
            }
            catch (...)
            {
            }
        }
    }
    P().requestQuit();
}

}  // namespace ll
