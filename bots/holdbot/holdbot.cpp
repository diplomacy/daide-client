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
#include "holdbot.h"

void HoldBot::send_nme_or_obs() {
    send_name_and_version_to_server(BOT_FAMILY, BOT_GENERATION);
}

void HoldBot::process_now_message(TokenMessage &incoming_message) {
    MapAndUnits::UNIT_SET::iterator unit_iterator;

    if ((m_map_and_units->current_season == TOKEN_SEASON_SPR)
        || (m_map_and_units->current_season == TOKEN_SEASON_FAL)) {
        // Order all units to hold.
        for (unit_iterator = m_map_and_units->our_units.begin();
             unit_iterator != m_map_and_units->our_units.end();
             unit_iterator++) {
            m_map_and_units->set_hold_order(*unit_iterator);
        }
    } else if ((m_map_and_units->current_season == TOKEN_SEASON_SUM)
               || (m_map_and_units->current_season == TOKEN_SEASON_AUT)) {
        // Order all dislodged units to disband.
        for (unit_iterator = m_map_and_units->our_dislodged_units.begin();
             unit_iterator != m_map_and_units->our_dislodged_units.end();
             unit_iterator++) {
            m_map_and_units->set_disband_order(*unit_iterator);
        }
    } else {
        if (m_map_and_units->our_units.size() > m_map_and_units->our_centres.size()) {
            // Too many units. Disband.
            while (m_map_and_units->our_units.size() > m_map_and_units->our_centres.size()) {
                m_map_and_units->set_remove_order(*(m_map_and_units->our_units.begin()));
                m_map_and_units->our_units.erase(m_map_and_units->our_units.begin());
            }
        } else if (m_map_and_units->our_units.size() < m_map_and_units->our_centres.size()) {
            // Not enough units. Waive builds.
            m_map_and_units->set_multiple_waive_orders(
                    m_map_and_units->our_centres.size() - m_map_and_units->our_units.size());
        }
    }

    send_orders_to_server();
}

