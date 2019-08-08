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

#include <iostream>
#include "bots/dumbbot/bot_type.h"
#include "bots/dumbbot/dumbbot.h"
#include "daide_client/error_log.h"
#include "daide_client/token_text_map.h"

using DAIDE::DumbBot;
using DAIDE::MapAndUnits;
using DAIDE::Token;
using DAIDE::TokenMessage;

const int NEUTRAL_POWER_INDEX = MapAndUnits::MAX_POWERS;

constexpr DumbBot::WEIGHTING DumbBot::m_spring_proximity_weight[PROXIMITY_DEPTH];
constexpr DumbBot::WEIGHTING DumbBot::m_fall_proximity_weight[PROXIMITY_DEPTH];
constexpr DumbBot::WEIGHTING DumbBot::m_build_proximity_weight[PROXIMITY_DEPTH];
constexpr DumbBot::WEIGHTING DumbBot::m_remove_proximity_weight[PROXIMITY_DEPTH];

/**
 * How it works.
 *
 * DumbBot works in two stages. In the first stage it calculates a value for each province and each coast. Then in the
 * second stage, it works out an order for each unit, based on those values.
 *
 * For each province, it calculates the following :
 *
 * - If it is our supply centre, the size of the largest adjacent power
 * - If it is not our supply centre, the size of the owning power
 * - If it is not a supply centre, zero.
 *
 * Then
 *
 * Proximity[0] for each coast, is the above value, multiplied by a weighting.
 *
 * Then
 *
 * Proximity[n] for each coast, = ( sum( proximity[ n-1 ] for all adjacent coasts
 *                                * proximity[ n-1 ] for this coast ) / 5
 *
 * Also for each province, it works out the strength of attack we have on that province - the number of adjacent units
 * we have, and the competition for that province - the number of adjacent units any one other power has.
 *
 * Finally it works out an overall value for each coast, based on all the proximity values for that coast, and the
 * strength and competition for that province, each of which is multiplied by a weighting
 *
 * Then for move orders, it tries to move to the best adjacent coast, with a random chance of it moving to the second
 * best, third best, etc (with that chance varying depending how close to first the second place is, etc). If the best
 * place is where it already is, it holds. If the best place already has an occupying unit, or already has a unit
 * moving there, it either supports that unit, or moves elsewhere, depending on whether the other unit is guaranteed
 * to succeed or not.
 *
 * Retreats are the same, except there are no supports...
 *
 * Builds and Disbands are also much the same - build in the highest value home centre disband the unit in the lowest
 * value location.
 *
 **/
DumbBot::DumbBot() :
    m_attack_value {},
    m_defense_value {},
    m_power_size {},
    m_strength_value {},
    m_competition_value {}
{}

void DumbBot::send_nme_or_obs() {
    send_name_and_version_to_server(BOT_FAMILY, BOT_GENERATION);
}

void DumbBot::process_mdf_message(const TokenMessage & /*incoming_msg*/) {
    MapAndUnits::PROVINCE_COASTS *province_coasts {nullptr};
    MapAndUnits::COAST_SET *adjacent_coasts {nullptr};

    // Build the set of adjacent provinces
    for (int province_ctr = 0; province_ctr < m_map_and_units->number_of_provinces; province_ctr++) {
        province_coasts = &(m_map_and_units->game_map[province_ctr].coast_info);

        for (auto &province_coast : *province_coasts) {
            adjacent_coasts = &(province_coast.second.adjacent_coasts);

            for (const auto &adjacent_coast : *adjacent_coasts) {
                m_adjacent_provinces[province_ctr].insert(adjacent_coast.province_index);
            }
        }
    }
}

void DumbBot::process_sco_message(const TokenMessage & /*incoming_msg*/) {
    for (int &power_size : m_power_size) {
        power_size = 0;
    }

    // Count the size of each power
    for (int province_ctr = 0; province_ctr < m_map_and_units->number_of_provinces; province_ctr++) {
        if (m_map_and_units->game_map[province_ctr].is_supply_centre) {
            m_power_size[get_power_index(m_map_and_units->game_map[province_ctr].owner)]++;
        }
    }

    // Modify it using the Ax^2 + Bx + C formula
    for (int &power_size : m_power_size) {
        power_size = m_size_square_coefficient * power_size * power_size
                     + m_size_coefficient * power_size
                     + m_size_constant;
    }
}

// Calculate the defense value. The defense value of a centre is the size of the largest enemy who has a unit which can
// move into the province
DumbBot::WEIGHTING DumbBot::calculate_defense_value(int province_index) {
    WEIGHTING defense_value {0};
    MapAndUnits::COAST_ID coast_id {-1};
    MapAndUnits::COAST_SET *adjacent_unit_adjacent_coasts {nullptr};

    // For each adjacent province
    for (auto adjacent_province_itr = m_adjacent_provinces[province_index].begin();
         adjacent_province_itr != m_adjacent_provinces[province_index].end();
         adjacent_province_itr++) {

        // If there is a unit there
        auto adjacent_unit_itr = m_map_and_units->units.find(*adjacent_province_itr);
        if (adjacent_unit_itr != m_map_and_units->units.end()) {

            // If it would increase the defense value
            if ((m_power_size[adjacent_unit_itr->second.nationality] > defense_value)
                && (adjacent_unit_itr->second.nationality != m_map_and_units->power_played.get_subtoken())) {

                // If it can move to this province
                adjacent_unit_adjacent_coasts = &(m_map_and_units->game_map[*adjacent_province_itr]
                                                  .coast_info[adjacent_unit_itr->second.coast_id.coast_token]
                                                  .adjacent_coasts);
                coast_id.province_index = province_index;
                coast_id.coast_token = Token(0);

                // Update the defense value
                if ((adjacent_unit_adjacent_coasts->lower_bound(coast_id) != adjacent_unit_adjacent_coasts->end())
                    && (adjacent_unit_adjacent_coasts->lower_bound(coast_id)->province_index == province_index)) {

                    defense_value = m_power_size[adjacent_unit_itr->second.nationality];
                }
            }
        }
    }

    return defense_value;
}

int DumbBot::get_power_index(const Token &power_token) {
    int power_index {-1};

    if (power_token.get_category() == DAIDE::CATEGORY_POWER) {
        power_index = power_token.get_subtoken();
    } else {
        power_index = NEUTRAL_POWER_INDEX;
    }

    return power_index;
}

void DumbBot::process_now_message(const TokenMessage & /*incoming_msg*/) {

    // Spring Moves/Retreats
    if ((m_map_and_units->current_season == DAIDE::TOKEN_SEASON_SPR)
        || (m_map_and_units->current_season == DAIDE::TOKEN_SEASON_SUM)) {

        calculate_factors(m_proximity_spring_attack_weight, m_proximity_spring_defense_weight);
        calculate_dest_value(m_spring_proximity_weight, m_spring_strength_weight, m_spring_competition_weight);

    // Fall Moves/Retreats
    } else if ((m_map_and_units->current_season == DAIDE::TOKEN_SEASON_FAL)
               || (m_map_and_units->current_season == DAIDE::TOKEN_SEASON_AUT)) {

        calculate_factors(m_proximity_fall_attack_weight, m_proximity_fall_defense_weight);
        calculate_dest_value(m_fall_proximity_weight, m_fall_strength_weight, m_fall_competition_weight);

    // Adjustments
    } else {
        calculate_factors(m_proximity_spring_attack_weight, m_proximity_spring_defense_weight);

        // Disbanding
        if (m_map_and_units->our_units.size() > m_map_and_units->our_centres.size()) {
            calculate_winter_dest_value(m_remove_proximity_weight, m_remove_defense_weight);

        // Building
        } else {
            calculate_winter_dest_value(m_build_proximity_weight, m_build_defense_weight);
        }
    }

    // Movement
    if ((m_map_and_units->current_season == DAIDE::TOKEN_SEASON_SPR)
            || (m_map_and_units->current_season == DAIDE::TOKEN_SEASON_FAL)) {
        generate_movement_orders();

    // Retreats
    } else if ((m_map_and_units->current_season == DAIDE::TOKEN_SEASON_SUM)
               || (m_map_and_units->current_season == DAIDE::TOKEN_SEASON_AUT)) {
        generate_retreat_orders();

    // Adjustments
    } else {
        // Too many units. Remove.
        if (m_map_and_units->our_units.size() > m_map_and_units->our_centres.size()) {
            generate_remove_orders(m_map_and_units->our_units.size() - m_map_and_units->our_centres.size());

        // Not enough units. Builds.
        } else if (m_map_and_units->our_units.size() < m_map_and_units->our_centres.size()) {
            generate_build_orders(m_map_and_units->our_centres.size() - m_map_and_units->our_units.size());
        }
    }

    // Submitting orders
    send_orders_to_server();
}

void DumbBot::calculate_factors(const WEIGHTING proximity_attack_weight, const WEIGHTING proximity_defense_weight) {
    const int DIVISION_FACTOR = 5;
    int previous_province {-1};
    int adjacent_unit_count[MapAndUnits::MAX_PROVINCES][MapAndUnits::MAX_POWERS] {};
    WEIGHTING previous_weight {-1};
    MapAndUnits::COAST_ID coast_id {-1};
    MapAndUnits::PROVINCE_COASTS *province_coasts {nullptr};
    MapAndUnits::COAST_SET *adjacent_coasts {nullptr};

    // Initialise arrays to 0
    for (int province_ctr = 0; province_ctr < m_map_and_units->number_of_provinces; province_ctr++) {
        m_attack_value[province_ctr] = 0;
        m_defense_value[province_ctr] = 0;
    }

    // Calculate attack and defense values for each province
    for (int province_ctr = 0; province_ctr < m_map_and_units->number_of_provinces; province_ctr++) {
        if (m_map_and_units->game_map[province_ctr].is_supply_centre) {

            // Our SC. Calc defense value
            if (m_map_and_units->game_map[province_ctr].owner == m_map_and_units->power_played) {
                m_defense_value[province_ctr] = calculate_defense_value(province_ctr);

            // Not ours. Calc attack value (which is the size of the owning power)
            } else {
                m_attack_value[province_ctr] = m_power_size[get_power_index(
                        m_map_and_units->game_map[province_ctr].owner)];
            }
        }
    }

    // Calculate proximities[ 0 ].
    // Proximity[0] is calculated based on the attack value and defense value of the province,
    // modified by the weightings for the current season.
    for (int province_ctr = 0; province_ctr < m_map_and_units->number_of_provinces; province_ctr++) {
        province_coasts = &(m_map_and_units->game_map[province_ctr].coast_info);
        coast_id.province_index = province_ctr;

        for (auto &province_coast : *province_coasts) {
            coast_id.coast_token = province_coast.first;
            (m_proximity_map[0])[coast_id] = m_attack_value[province_ctr] * proximity_attack_weight
                                             + m_defense_value[province_ctr] * proximity_defense_weight;

            // Clear proximity[ 1... ]
            for (int proximity_ctr = 1; proximity_ctr < PROXIMITY_DEPTH; proximity_ctr++) {
                (m_proximity_map[proximity_ctr])[coast_id] = 0;
            }
        }
    }

    // Calculate proximities [ 1... ]
    // proximity[n] = ( sum of proximity[n-1] for this coast plus all coasts
    // which this coast is adjacent to ) / 5
    // The divide by 5 is just to keep all values of proximity[ n ] in the same range.
    // The average coast has 4 adjacent coasts, plus itself.
    for (int proximity_ctr = 1; proximity_ctr < PROXIMITY_DEPTH; proximity_ctr++) {
        for (auto proximity_itr = m_proximity_map[proximity_ctr].begin();
             proximity_itr != m_proximity_map[proximity_ctr].end();
             proximity_itr++) {

            adjacent_coasts = &(m_map_and_units->game_map[proximity_itr->first.province_index]
                                .coast_info[proximity_itr->first.coast_token]
                                .adjacent_coasts);
            previous_province = -1;

            for (const auto &adjacent_coast : *adjacent_coasts) {

                // If it is the same province again (e.g. This is Spa/sc after Spa/nc, for MAO
                if (adjacent_coast.province_index == previous_province) {

                    // Replace the previous weight with the new one, if the new one is higher
                    if ((m_proximity_map[proximity_ctr - 1])[adjacent_coast] > previous_weight) {
                        proximity_itr->second = proximity_itr->second - previous_weight;
                        previous_weight = (m_proximity_map[proximity_ctr - 1])[adjacent_coast];
                        proximity_itr->second = proximity_itr->second + previous_weight;
                    }

                // Add to the weight
                } else {
                    previous_weight = (m_proximity_map[proximity_ctr - 1])[adjacent_coast];
                    proximity_itr->second = proximity_itr->second + previous_weight;
                }

                previous_province = adjacent_coast.province_index;
            }

            // Add this province in, then divide the answer by 5
            proximity_itr->second = proximity_itr->second
                                    + (m_proximity_map[proximity_ctr - 1])[proximity_itr->first];
            proximity_itr->second = proximity_itr->second / DIVISION_FACTOR;
        }
    }

    // Calculate number of units each power has next to each province
    for (int province_ctr = 0; province_ctr < m_map_and_units->number_of_provinces; province_ctr++) {
        for (int power_ctr = 0; power_ctr < m_map_and_units->number_of_powers; power_ctr++) {
            adjacent_unit_count[province_ctr][power_ctr] = 0;
        }

        // Strength value is the number of units we have next to the province
        m_strength_value[province_ctr] = 0;

        // Competition value is the greatest number of units any other power has next to the province
        m_competition_value[province_ctr] = 0;
    }

    for (auto &unit : m_map_and_units->units) {
        adjacent_coasts = &(m_map_and_units->game_map[unit.second.coast_id.province_index]
                            .coast_info[unit.second.coast_id.coast_token]
                            .adjacent_coasts);

        for (const auto &adjacent_coast : *adjacent_coasts) {
            adjacent_unit_count[adjacent_coast.province_index][unit.second.nationality]++;
        }
        adjacent_unit_count[unit.second.coast_id.province_index][unit.second.nationality]++;
    }

    for (int province_ctr = 0; province_ctr < m_map_and_units->number_of_provinces; province_ctr++) {
        for (int power_ctr = 0; power_ctr < m_map_and_units->number_of_powers; power_ctr++) {
            if (power_ctr == m_map_and_units->power_played.get_subtoken()) {
                m_strength_value[province_ctr] = adjacent_unit_count[province_ctr][power_ctr];
            } else if (adjacent_unit_count[province_ctr][power_ctr] > m_competition_value[province_ctr]) {
                m_competition_value[province_ctr] = adjacent_unit_count[province_ctr][power_ctr];
            }
        }
    }
}

// Given the province and coast calculated values, and the weighting for this turn, calculate
// the value of each coast.
void DumbBot::calculate_dest_value(const WEIGHTING *proximity_weight,
                                   const WEIGHTING strength_weight,
                                   const WEIGHTING competition_weight) {
    MapAndUnits::COAST_ID coast_id {-1};
    WEIGHTING destination_weight {0};

    for (auto &proximity_itr : m_proximity_map[0]) {
        coast_id = proximity_itr.first;
        destination_weight = 0;

        for (int proximity_ctr = 0; proximity_ctr < PROXIMITY_DEPTH; proximity_ctr++) {
            destination_weight += (m_proximity_map[proximity_ctr])[coast_id] * proximity_weight[proximity_ctr];
        }

        destination_weight += strength_weight * m_strength_value[coast_id.province_index];
        destination_weight -= competition_weight * m_competition_value[coast_id.province_index];
        m_destination_value[coast_id] = destination_weight;
    }
}

// Given the province and coast calculated values, and the weighting for this turn, calculate
// the value of each coast for winter builds and removals.
void DumbBot::calculate_winter_dest_value(const WEIGHTING *proximity_weight, const WEIGHTING defense_weight) {
    MapAndUnits::COAST_ID coast_id {-1};
    WEIGHTING destination_weight {0};

    for (auto &proximity_itr : m_proximity_map[0]) {
        coast_id = proximity_itr.first;
        destination_weight = 0;

        for (int proximity_ctr = 0; proximity_ctr < PROXIMITY_DEPTH; proximity_ctr++) {
            destination_weight += (m_proximity_map[proximity_ctr])[coast_id] * proximity_weight[proximity_ctr];
        }

        destination_weight += defense_weight * m_defense_value[coast_id.province_index];
        m_destination_value[coast_id] = destination_weight;
    }
}

// Generate the actual orders for a movement turn
void DumbBot::generate_movement_orders() {
    const int MAX_PERC = 100;
    bool try_next_province {false};
    bool selection_is_ok {false};
    bool order_unit_to_move {false};
    int occupying_unit_order {-1};
    WEIGHTING next_province_chance {-1};
    DESTINATION_MAP destination_map;
    MOVING_UNIT_MAP moving_unit_map;
    MapAndUnits::UNIT_AND_ORDER *unit {nullptr};
    MapAndUnits::COAST_SET *adjacent_coasts {nullptr};

    // Put our units into a random order. This is one of the ways in which DumbBot is made non-deterministic -
    // the order the units are considered in can affect the orders selected
    generate_random_unit_list(m_map_and_units->our_units);

    auto unit_itr = m_random_unit_map.begin();
    while (unit_itr != m_random_unit_map.end()) {

        // Build the list of possible destinations for this unit, indexed by the weight for the destination
        destination_map.clear();
        unit = &(m_map_and_units->units[unit_itr->second]);

        // Put all the adjacent coasts into the destination map
        adjacent_coasts = &(m_map_and_units->game_map[unit_itr->second]
                            .coast_info[unit->coast_id.coast_token]
                            .adjacent_coasts);

        for (const auto & adjacent_coast : *adjacent_coasts) {
            destination_map.insert(DESTINATION_MAP::value_type(m_destination_value[adjacent_coast], adjacent_coast));
        }

        // Put the current location in (we can hold rather than move)
        destination_map.insert(DESTINATION_MAP::value_type(m_destination_value[unit->coast_id] + 1, unit->coast_id));

        do {
            // Pick a destination
            auto destination_itr = destination_map.begin();
            try_next_province = true;

            while (try_next_province) {
                // Consider the next destination in the map
                auto next_destination_itr = destination_itr;
                next_destination_itr++;

                // If there is no next destination, give up.
                if (next_destination_itr == destination_map.end()) {
                    try_next_province = false;

                } else {
                    // Randomly determine whether to use the current or next destination,
                    // based on their relative weights (the higher the difference, the
                    // more chance of picking the better one).
                    if (destination_itr->first == 0) {
                        next_province_chance = 0;
                    } else {
                        next_province_chance = ((destination_itr->first - next_destination_itr->first) *
                                                m_alternative_difference_modifier)
                                               / destination_itr->first;
                    }

                    // Pick the next one (and go around the loop again to keep considering more options
                    if ((rand_no(MAX_PERC) < m_play_alternative) && (rand_no(MAX_PERC) >= next_province_chance)) {
                        destination_itr = next_destination_itr;

                    // Use this one
                    } else {
                        try_next_province = false;
                    }
                }
            }

            // Check whether this is a reasonable selection
            selection_is_ok = true;
            order_unit_to_move = true;

            // If this is a hold order
            if (destination_itr->second.province_index == unit->coast_id.province_index) {
                m_map_and_units->set_hold_order(unit_itr->second);

            // If there is a unit in this province already
            } else {
                auto current_unit_itr = m_map_and_units->units.find(destination_itr->second.province_index);
                if (m_map_and_units->our_units.find(destination_itr->second.province_index) !=
                    m_map_and_units->our_units.end()) {

                    // If it is not yet ordered
                    if (current_unit_itr->second.order_type == MapAndUnits::NO_ORDER) {

                        // Find this unit in the random unit map
                        for (auto &random_unit_itr : m_random_unit_map) {
                            if (random_unit_itr.second == destination_itr->second.province_index) {
                                occupying_unit_order = random_unit_itr.first;
                            }
                        }

                        // Move this unit to after the one it's waiting for. We can't decide
                        // whether to move there or not, so give up on this unit for now
                        if (occupying_unit_order >= 0) {
                            m_random_unit_map.insert(
                                    RANDOM_UNIT_MAP::value_type(occupying_unit_order - 1, unit_itr->second));
                            order_unit_to_move = false;
                        }

                    // If it is not moving
                    } else if ((current_unit_itr->second.order_type != MapAndUnits::MOVE_ORDER)
                               && (current_unit_itr->second.order_type != MapAndUnits::MOVE_BY_CONVOY_ORDER)) {

                        // If it needs supporting, support it
                        if (m_competition_value[destination_itr->second.province_index] > 1) {
                            m_map_and_units->set_support_to_hold_order(unit_itr->second,
                                                                       destination_itr->second.province_index);

                            order_unit_to_move = false;

                        // This is not an acceptable selection
                        } else {
                            selection_is_ok = false;

                            // Make sure it isn't selected again
                            destination_map.erase(destination_itr);
                        }
                    }
                }

                // If there is a unit already moving to this province
                auto moving_unit_itr = moving_unit_map.find(destination_itr->second.province_index);
                if (moving_unit_itr != moving_unit_map.end()) {

                    // If it may need support, support it
                    if (m_competition_value[destination_itr->second.province_index] > 0) {

                        // Support it
                        m_map_and_units->set_support_to_move_order(unit_itr->second, moving_unit_itr->second,
                                                                   moving_unit_itr->first);
                        order_unit_to_move = false;

                    // This is not an acceptable selection
                    // Make sure it isn't selected again
                    } else {
                        selection_is_ok = false;
                        destination_map.erase(destination_itr);
                    }
                }

                if ((selection_is_ok) && (order_unit_to_move)) {
                    m_map_and_units->set_move_order(unit_itr->second, destination_itr->second);
                    moving_unit_map[destination_itr->second.province_index] = unit_itr->second;
                }
            }
        } while (!selection_is_ok);

        // Unit is now ordered, so delete it from the random unit map
        m_random_unit_map.erase(unit_itr);

        // Move onto the next one
        unit_itr = m_random_unit_map.begin();
    }

    check_for_wasted_holds(moving_unit_map);
}

void DumbBot::check_for_wasted_holds(MOVING_UNIT_MAP &moving_unit_map) {
    WEIGHTING max_destination_value {-1};
    MapAndUnits::PROVINCE_INDEX destination {-1};
    MapAndUnits::PROVINCE_INDEX source {-1};
    MapAndUnits::UNIT_AND_ORDER *unit {nullptr};
    MapAndUnits::COAST_SET *adjacent_coasts {nullptr};

    // For each unit, if it is ordered to hold
    for (int our_unit : m_map_and_units->our_units) {
        unit = &(m_map_and_units->units[our_unit]);

        if (unit->order_type == MapAndUnits::HOLD_ORDER) {

            // Consider every province we can move to
            adjacent_coasts = &(m_map_and_units->game_map[unit->coast_id.province_index]
                                .coast_info[unit->coast_id.coast_token]
                                .adjacent_coasts);
            max_destination_value = 0;

            for (const auto &adjacent_coast : *adjacent_coasts) {

                // Check if there is a unit moving there
                if (moving_unit_map.find(adjacent_coast.province_index) != moving_unit_map.end()) {

                    // Unit needs support
                    if (m_competition_value[adjacent_coast.province_index] > 0) {

                        // Best so far
                        if (m_destination_value[adjacent_coast] > max_destination_value) {
                            max_destination_value = m_destination_value[adjacent_coast];
                            destination = adjacent_coast.province_index;
                            source = moving_unit_map[adjacent_coast.province_index];
                        }
                    }

                // Check if there is a unit holding there
                } else {
                    auto adjacent_unit_itr = m_map_and_units->units.find(adjacent_coast.province_index);
                    if ((adjacent_unit_itr != m_map_and_units->units.end())
                        && (adjacent_unit_itr->second.nationality == m_map_and_units->power_played.get_subtoken())
                        && (adjacent_unit_itr->second.order_type != MapAndUnits::MOVE_ORDER)
                        && (adjacent_unit_itr->second.order_type != MapAndUnits::MOVE_BY_CONVOY_ORDER)) {

                        // Unit needs support
                        if (m_competition_value[adjacent_coast.province_index] > 1) {

                            // Best so far
                            if (m_destination_value[adjacent_coast] > max_destination_value) {
                                max_destination_value = m_destination_value[adjacent_coast];
                                destination = adjacent_coast.province_index;
                                source = destination;
                            }
                        }
                    }
                }
            }

            // If something worth supporting was found, support it
            if (max_destination_value > 0) {
                if (source == destination) {
                    m_map_and_units->set_support_to_hold_order(unit->coast_id.province_index, destination);
                } else {
                    m_map_and_units->set_support_to_move_order(unit->coast_id.province_index, source, destination);
                }
            }
        }
    }
}

// Generate Retreat orders

void DumbBot::generate_retreat_orders() {
    const int MAX_PERC = 100;
    bool try_next_province {false};
    bool selection_is_ok {false};
    WEIGHTING next_province_chance {-1};
    DESTINATION_MAP destination_map {};
    MOVING_UNIT_MAP moving_unit_map {};
    MapAndUnits::UNIT_AND_ORDER *unit {nullptr};

    // Put the units into a list in random order
    generate_random_unit_list(m_map_and_units->our_dislodged_units);

    auto unit_itr = m_random_unit_map.begin();
    while (unit_itr != m_random_unit_map.end()) {
        destination_map.clear();
        unit = &(m_map_and_units->dislodged_units[unit_itr->second]);

        // Put all the adjacent coasts into the destination map
        for (const auto &retreat_option : unit->retreat_options) {
            destination_map.insert(DESTINATION_MAP::value_type(m_destination_value[retreat_option], retreat_option));
        }

        do {
            // Pick a destination
            auto destination_itr = destination_map.begin();

            // No retreat possible. Disband unit
            if (destination_itr == destination_map.end()) {
                m_map_and_units->set_disband_order(unit_itr->second);
                selection_is_ok = true;

            } else {
                // Determine whether to try the next option instead
                try_next_province = true;
                while (try_next_province) {
                    auto next_destination_itr = destination_itr;
                    next_destination_itr++;

                    // If there is no next option, don't try it...
                    if (next_destination_itr == destination_map.end()) {
                        try_next_province = false;

                    // Randomly decide whether to move on
                    } else {
                        if (destination_itr->first == 0) {
                            next_province_chance = 0;
                        } else {
                            next_province_chance = ((destination_itr->first - next_destination_itr->first) *
                                                    m_alternative_difference_modifier)
                                                   / destination_itr->first;
                        }

                        // Move on
                        if ((rand_no(MAX_PERC) < m_play_alternative) && (rand_no(MAX_PERC) >= next_province_chance)) {
                            destination_itr = next_destination_itr;

                        // Stick with this province
                        } else {
                            try_next_province = false;
                        }
                    }
                }

                // Check whether this is a reasonable selection
                selection_is_ok = true;

                // If there is a unit already moving to this province
                auto moving_unit_itr = moving_unit_map.find(destination_itr->second.province_index);
                if (moving_unit_itr != moving_unit_map.end()) {

                    // This is not an acceptable selection
                    selection_is_ok = false;

                    // Make sure it isn't selected again
                    destination_map.erase(destination_itr);
                }

                // Order the retreat
                if (selection_is_ok) {
                    m_map_and_units->set_retreat_order(unit_itr->second, destination_itr->second);
                    moving_unit_map[destination_itr->second.province_index] = unit_itr->second;
                }
            }
        } while (!selection_is_ok);

        // Unit is now ordered, so delete it from the random unit map
        m_random_unit_map.erase(unit_itr);

        // Move onto the next one
        unit_itr = m_random_unit_map.begin();
    }
}

// Generate the actual remove orders for an adjustment phase

void DumbBot::generate_remove_orders(int remove_count) {
    const int MAX_PERC = 100;
    bool try_next_remove {false};
    WEIGHTING next_remove_chance {-1};
    DESTINATION_MAP remove_map {};
    MapAndUnits::UNIT_AND_ORDER *unit {nullptr};

    // Put all the units into a removal map, ordered by value of their location
    for (int our_unit : m_map_and_units->our_units) {
        unit = &(m_map_and_units->units[our_unit]);
        remove_map[m_destination_value[unit->coast_id]] = unit->coast_id;
    }

    // For each required removal
    for (int remove_ctr = 0; remove_ctr < remove_count; remove_ctr++) {
        // Start with the best option (which is the end of the list)
        auto remove_itr = remove_map.rbegin();
        try_next_remove = true;

        // Determine whether to try the next option
        while (try_next_remove) {
            auto next_remove_itr = remove_itr;
            next_remove_itr++;

            // No next option - so don't try it.
            if (next_remove_itr == remove_map.rend()) {
                try_next_remove = false;

            // Decide randomly
            } else {
                if (remove_itr->first == 0) {
                    next_remove_chance = 0;
                } else {
                    next_remove_chance =
                            ((remove_itr->first - next_remove_itr->first) * m_alternative_difference_modifier)
                            / remove_itr->first;
                }

                // Try the next one
                if ((rand_no(MAX_PERC) < m_play_alternative) && (rand_no(MAX_PERC) >= next_remove_chance)) {
                    remove_itr = next_remove_itr;

                // Stick with this one
                } else {
                    try_next_remove = false;
                }
            }
        }

        // Order the removal
        m_map_and_units->set_remove_order(remove_itr->second.province_index);

        auto remove_forward_itr = remove_itr.base();
        remove_forward_itr--;
        remove_map.erase(remove_forward_itr);
    }
}

// Generate the actual build orders for an adjustment turn

void DumbBot::generate_build_orders(int build_count) {
    const int MAX_PERC = 100;
    bool try_next_build {false};
    WEIGHTING next_build_chance {-1};
    int build_province {-1};
    int builds_remaining = build_count;
    MapAndUnits::COAST_ID coast_id {-1};
    DESTINATION_MAP build_map {};
    MapAndUnits::PROVINCE_COASTS *province_coasts {nullptr};

    // Put all the coasts of all the home centres into a map
    for (int open_home_centre : m_map_and_units->open_home_centres) {
        province_coasts = &(m_map_and_units->game_map[open_home_centre].coast_info);

        for (auto &province_coast : *province_coasts) {
            coast_id.province_index = open_home_centre;
            coast_id.coast_token = province_coast.first;
            build_map.insert(DESTINATION_MAP::value_type(m_destination_value[coast_id], coast_id));
        }
    }

    // For each build, while we have a vacant home centre
    while (!build_map.empty() && (builds_remaining > 0)) {
        // Select the best location
        auto build_itr = build_map.begin();
        try_next_build = true;

        // Consider the next location
        while (try_next_build) {
            auto next_build_itr = build_itr;
            next_build_itr++;

            // There isn't one, so stick with this one
            if (next_build_itr == build_map.end()) {
                try_next_build = false;

            // Determine randomly
            } else {
                if (build_itr->first == 0) {
                    next_build_chance = 0;
                } else {
                    next_build_chance =
                            ((build_itr->first - next_build_itr->first) * m_alternative_difference_modifier)
                            / build_itr->first;
                }

                // Try the next one
                if ((rand_no(MAX_PERC) < m_play_alternative) && (rand_no(MAX_PERC) >= next_build_chance)) {
                    build_itr = next_build_itr;

                // Stick with this one
                } else {
                    try_next_build = false;
                }
            }
        }

        // Order the build
        m_map_and_units->set_build_order(build_itr->second);
        build_province = build_itr->second.province_index;
        build_itr = build_map.begin();

        // Remove all other build options for this province
        while (build_itr != build_map.end()) {
            if (build_itr->second.province_index == build_province) {
                build_itr = build_map.erase(build_itr);
            } else {
                build_itr++;
            }
        }

        builds_remaining--;
    }

    // If not all builds ordered, order some waives.
    if (builds_remaining > 0) {
        m_map_and_units->set_total_number_of_waive_orders(builds_remaining);
    }
}

// Generate a random unit map from a set of units

void DumbBot::generate_random_unit_list(const MapAndUnits::UNIT_SET &units) {
    int unit_index {-1};

    m_random_unit_map.clear();

    for (int unit : units) {
        unit_index = rand_no(RAND_MAX);
        m_random_unit_map.insert(RANDOM_UNIT_MAP::value_type(unit_index, unit));
    }
}

// Generate a random number

int DumbBot::rand_no(int max_value) {
    int answer = rand() % max_value;
    return answer;
}

bool DumbBot::extract_parameters(const std::string &command_line_a, DAIDE::COMMAND_LINE_PARAMETERS &parameters) {
    int search_start;                    // Position to start searching command line
    int param_start;                     // Start of the next parameter
    int param_end;                       // End of the next parameter
    char param_token;                    // The token specifying the parameter type
    std::string parameter;               // The parameter
    bool extracted_ok = true;            // Whether the parameters were OK

    parameters.ip_specified = false;
    parameters.name_specified = false;
    parameters.port_specified = false;
    parameters.log_level_specified = false;
    parameters.reconnection_specified = false;

    std::string m_command_line = command_line_a;

    // Strip the program name off the command line
    if (m_command_line[0] == '"') {
        // Program name is in quotes.
        param_start = m_command_line.find('"', 1);

        if (param_start != std::string::npos) {
            m_command_line = m_command_line.substr(param_start + 1);
        }
    } else {
        // Program name is not quoted, so is terminated by a space
        param_start = m_command_line.find(' ');

        if (param_start != std::string::npos) {
            m_command_line = m_command_line.substr(param_start);
        }
    }

    param_start = m_command_line.find('-', 0);

    while (param_start != std::string::npos) {
        param_token = m_command_line[param_start + 1];

        param_end = m_command_line.find(' ', param_start);

        if (param_end == std::string::npos) {
            parameter = m_command_line.substr(param_start + 2);

            search_start = m_command_line.size();
        } else {
            parameter = m_command_line.substr(0, param_end).substr(param_start + 2);

            search_start = param_end;
        }

        switch (param_token) {
            case 's': {
                parameters.name_specified = true;
                parameters.server_name = parameter;

                break;
            }

            case 'i': {
                parameters.ip_specified = true;
                parameters.server_name = parameter;

                break;
            }

            case 'p': {
                parameters.port_specified = true;
                parameters.port_number = stoi(parameter);

                break;
            }

            case 'l': {
                parameters.log_level_specified = true;
                parameters.log_level = stoi(parameter);

                break;
            }

            case 'r': {
                if (parameter[3] == ':') {
                    parameters.reconnection_specified = true;
                    parameters.reconnect_power = parameter.substr(0, 3);
                    for (auto &c : parameters.reconnect_power) { c = toupper(c); }
                    parameters.reconnect_passcode = stoi(parameter.substr(4));
                } else {
                    std::cout << "-r should be followed by 'POW:passcode'\n"
                                 "POW should be three characters" << std::endl;
                }

                break;
            }

            default: {
                std::cout << std::string(BOT_FAMILY) << " - version " << std::string(BOT_GENERATION) << std::endl;
                std::cout << "Usage: " << std::string(BOT_FAMILY) <<
                             " [-sServerName|-iIPAddress] [-pPortNumber] "
                             "[-lLogLevel] [-rPOW:passcode] [-d]" << std::endl;
                extracted_ok = false;
            }
        }

        param_start = m_command_line.find('-', search_start);
    }

    if ((parameters.ip_specified) && (parameters.name_specified)) {
        std::cout << "You must not specify Server name and IP address" << std::endl;

        extracted_ok = false;
    }

    return extracted_ok;
}
