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
#include "bot_type.h"
#include "holdbot.h"

using DAIDE::TokenMessage;

void HoldBot::send_nme_or_obs() {
    send_name_and_version_to_server(BOT_FAMILY, BOT_GENERATION);
}

void HoldBot::process_now_message(const TokenMessage & /*incoming_message*/) {

    // Movement - Order all units to hold.
    if ((m_map_and_units->current_season == DAIDE::TOKEN_SEASON_SPR)
        || (m_map_and_units->current_season == DAIDE::TOKEN_SEASON_FAL)) {

        for (int our_unit : m_map_and_units->our_units) {
            m_map_and_units->set_hold_order(our_unit);
        }

    // Retreat - Order all dislodged units to disband.
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

        // Not enough units. Waive builds.
        } else if (m_map_and_units->our_units.size() < m_map_and_units->our_centres.size()) {
            m_map_and_units->set_multiple_waive_orders(
                    m_map_and_units->our_centres.size() - m_map_and_units->our_units.size());
        }
    }

    // Submitting orders
    send_orders_to_server();
}
