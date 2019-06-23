/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * Definition of the Bot to use.
 *
 * (C) David Norman 2002 david@ellought.demon.co.uk
 *
 * This software may be reused for non-commercial purposes without charge, and
 * without notifying the author. Use of any part of this software for commercial 
 * purposes without permission from the Author is prohibited.
 *
 * Modified by John Newbury
 *
 * Release 8~2~b
 **/

#include "daide_client/map_and_units.h"
#include "daide_client/stdafx.h"
#include "bot_type.h"
#include "randbot.h"

void RandBot::send_nme_or_obs() {
    send_name_and_version_to_server(BOT_FAMILY, BOT_GENERATION);
}

template<class SetType>
typename SetType::value_type get_random_set_member(SetType my_set) {
    int set_size = my_set.size();
    int rand_selection = rand() / 37 % set_size;
    int item_counter;
    SetType::iterator set_itr;

    set_itr = my_set.begin();

    for (item_counter = 0; item_counter < rand_selection; item_counter++) {
        set_itr++;
    }

    return *set_itr;
}

void RandBot::process_now_message(TokenMessage &incoming_message) {
    MapAndUnits::UNIT_SET::iterator unit_itr;
    MapAndUnits::UNIT_AND_ORDER *unit_info;
    MapAndUnits::COAST_DETAILS *province_coast_info;
    MapAndUnits::COAST_ID move_destination;
    int number_of_builds;
    MapAndUnits::PROVINCE_COASTS *build_coast_info;
    MapAndUnits::COAST_ID build_location;

    if (!m_map_and_units->game_over) {
        if ((m_map_and_units->current_season == TOKEN_SEASON_SPR)
            || (m_map_and_units->current_season == TOKEN_SEASON_FAL)) {
            // Order all units to hold.
            for (unit_itr = m_map_and_units->our_units.begin();
                 unit_itr != m_map_and_units->our_units.end();
                 unit_itr++) {
                unit_info = &(m_map_and_units->units[*unit_itr]);
                province_coast_info = &(m_map_and_units->game_map[unit_info->coast_id.province_index]
                        .coast_info[unit_info->coast_id.coast_token]);
                move_destination = get_random_set_member<MapAndUnits::COAST_SET>(province_coast_info->adjacent_coasts);
                m_map_and_units->set_move_order(*unit_itr, move_destination);
            }
        } else if ((m_map_and_units->current_season == TOKEN_SEASON_SUM)
                   || (m_map_and_units->current_season == TOKEN_SEASON_AUT)) {
            // Order all dislodged units to disband.
            for (unit_itr = m_map_and_units->our_dislodged_units.begin();
                 unit_itr != m_map_and_units->our_dislodged_units.end();
                 unit_itr++) {
                m_map_and_units->set_disband_order(*unit_itr);
            }
        } else {
            if (m_map_and_units->our_units.size() > m_map_and_units->our_centres.size()) {
                // Too many units. Disband.
                while (m_map_and_units->our_units.size() > m_map_and_units->our_centres.size()) {
                    m_map_and_units->set_remove_order(*(m_map_and_units->our_units.begin()));
                    m_map_and_units->our_units.erase(m_map_and_units->our_units.begin());
                }
            } else if (m_map_and_units->our_units.size() < m_map_and_units->our_centres.size()) {
                number_of_builds = m_map_and_units->our_centres.size() - m_map_and_units->our_units.size();

                while ((number_of_builds > 0) && !m_map_and_units->open_home_centres.empty()) {
                    build_location.province_index = get_random_set_member<MapAndUnits::PROVINCE_SET>(
                            m_map_and_units->open_home_centres);
                    build_coast_info = &(m_map_and_units->game_map[build_location.province_index].coast_info);
                    build_location.coast_token = get_random_set_member<MapAndUnits::PROVINCE_COASTS>(
                            *build_coast_info).first;

                    m_map_and_units->set_build_order(build_location);

                    number_of_builds--;
                    m_map_and_units->open_home_centres.erase(build_location.province_index);
                }

                m_map_and_units->set_multiple_waive_orders(number_of_builds);
            }
        }

        send_orders_to_server();
    }
}

