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
 * Release 8~3
 **/

#include "bot_type.h"
#include "randbot.h"
#include "daide_client/map_and_units.h"

using DAIDE::RandBot;
using DAIDE::MapAndUnits;
using DAIDE::TokenMessage;

void RandBot::send_nme_or_obs() {
    send_name_and_version_to_server(get_bot_name(), BOT_GENERATION);
}

template<class SetType>
typename SetType::value_type get_random_set_member(SetType my_set) {
    int set_size = my_set.size();
    int rand_selection = rand() / 37 % set_size;

    auto set_itr = my_set.begin();
    for (int item_ctr = 0; item_ctr < rand_selection; item_ctr++, set_itr++) {}
    return *set_itr;
}

void RandBot::process_now_message(const TokenMessage & /*incoming_message*/) {
    int number_of_builds {0};
    MapAndUnits::COAST_ID move_destination {-1};
    MapAndUnits::COAST_ID build_location {-1};
    MapAndUnits::UNIT_AND_ORDER *unit_info {nullptr};
    MapAndUnits::COAST_DETAILS *province_coast_info {nullptr};
    MapAndUnits::PROVINCE_COASTS *build_coast_info {nullptr};

    if (!m_map_and_units->game_over) {
        send_message_to_server(TOKEN_COMMAND_NOT & TOKEN_COMMAND_GOF);

        // Movement - Order units to move randomly.
        if ((m_map_and_units->current_season == DAIDE::TOKEN_SEASON_SPR)
            || (m_map_and_units->current_season == DAIDE::TOKEN_SEASON_FAL)) {

            for (int our_unit : m_map_and_units->our_units) {
                unit_info = &(m_map_and_units->units[our_unit]);
                province_coast_info = &(m_map_and_units->game_map[unit_info->coast_id.province_index]
                                        .coast_info[unit_info->coast_id.coast_token]);
                move_destination = get_random_set_member<MapAndUnits::COAST_SET>(province_coast_info->adjacent_coasts);
                m_map_and_units->set_move_order(our_unit, move_destination);
            }

        // Retreats - Order all dislodged units to disband.
        } else if ((m_map_and_units->current_season == DAIDE::TOKEN_SEASON_SUM)
                   || (m_map_and_units->current_season == DAIDE::TOKEN_SEASON_AUT)) {

            for (int our_dislodged_unit : m_map_and_units->our_dislodged_units) {
                m_map_and_units->set_disband_order(our_dislodged_unit);
            }

        // Adjustment
        } else {

            // Too many units. Disband.
            if (m_map_and_units->our_units.size() > m_map_and_units->our_centres.size()) {
                while (m_map_and_units->our_units.size() > m_map_and_units->our_centres.size()) {
                    m_map_and_units->set_remove_order(*(m_map_and_units->our_units.begin()));
                    m_map_and_units->our_units.erase(m_map_and_units->our_units.begin());
                }

            // Building units randomly
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

        // Submitting orders
        send_orders_to_server();
        send_message_to_server(TOKEN_COMMAND_GOF);
    }
}
