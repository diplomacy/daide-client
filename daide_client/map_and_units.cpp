/**
 * Diplomacy AI Client & Server - Part of the DAIDE project.
 *
 * Map And Unit Handling Code. Allows the map to be set up, and orders entered.
 *
 * (C) David Norman 2002 david@ellought.demon.co.uk
 *
 * This software may be reused for non-commercial purposes without charge, and
 * without notifying the author. Use of any part of this software for commercial 
 * purposes without permission from the Author is prohibited.
 *
 * Release 8~2
 **/

#include "map_and_units.h"
#include "token_message.h"

using DAIDE::MapAndUnits;
using DAIDE::Token;
using DAIDE::TokenMessage;

// Function to get the instance of the MapAndUnits object
MapAndUnits *MapAndUnits::get_instance() {
    static MapAndUnits object_instance;
    return &object_instance;
}

// Get a duplicate copy of the MapAndUnits class. Recommended for messing around with
// without affecting the master copy
MapAndUnits *MapAndUnits::get_duplicate_instance() {
    MapAndUnits *original {nullptr};
    MapAndUnits *duplicate {nullptr};

    // FIXME - Should return a smart pointer
    original = get_instance();
    duplicate = new MapAndUnits;
    *duplicate = *original;
    return duplicate;
}

// Delete a duplicate
void MapAndUnits::delete_duplicate_instance(MapAndUnits *duplicate) {
    // FIXME - Not needed if smart pointer used
    delete duplicate;
}

// Private constructor. Use get_instance to get the object
MapAndUnits::MapAndUnits() :
    number_of_provinces {NO_MAP},
    number_of_powers {NO_MAP},
    power_played {Token(0)},
    passcode {0},
    game_started {false},
    game_over {false},
    current_year {0},
    check_orders_on_submission {true},
    check_orders_on_adjudication {false},
    number_of_disbands {0}
{}

int MapAndUnits::set_map(const TokenMessage &mdf_message) {
    int error_location {ADJUDICATOR_NO_ERROR};      // location of error in the message, or ADJUDICATOR_NO_ERROR
    TokenMessage mdf_command {};                    // The command
    TokenMessage power_list {};                     // The list of powers
    TokenMessage provinces {};                      // The provinces
    TokenMessage adjacencies {};                    // The adjacencies

    // Check there are 4 submessages
    if (mdf_message.get_submessage_count() != 4) {
        return 0;               // error location is 0
    }

    // Get the four parts of the message
    mdf_command = mdf_message.get_submessage(0);
    power_list = mdf_message.get_submessage(1);
    provinces = mdf_message.get_submessage(2);
    adjacencies = mdf_message.get_submessage(3);

    // Check the mdf command
    if (!mdf_command.is_single_token() || mdf_command.get_token() != TOKEN_COMMAND_MDF) {
        return 0;               // error location is 0
    }

    // Process power list
    error_location = process_power_list(power_list);
    if (error_location != ADJUDICATOR_NO_ERROR) { return error_location; }

    // Process provinces
    error_location = process_provinces(provinces);
    if (error_location != ADJUDICATOR_NO_ERROR) { return error_location; }

    // Process adjacencies
    error_location = process_adjacencies(adjacencies);
    if (error_location != ADJUDICATOR_NO_ERROR) { return error_location; }

    // No errors
    return error_location;
}

int MapAndUnits::process_power_list(const TokenMessage &power_list) {
    bool power_used[MAX_POWERS] {};
    int error_location {ADJUDICATOR_NO_ERROR};
    Token power {};

    number_of_powers = power_list.get_message_length();

    // Marking all powers as false
    for (int power_ctr = 0; power_ctr < number_of_powers; power_ctr++) {
        power_used[power_ctr] = false;
    }

    // Making sure powers are valid and not used twice
    for (int power_ctr = 0; power_ctr < number_of_powers; power_ctr++) {
        power = power_list.get_token(power_ctr);
        if ((power.get_subtoken() >= number_of_powers) || (power_used[power.get_subtoken()])) {
            error_location = power_ctr;
        } else {
            power_used[power.get_subtoken()] = true;
        }
        if (error_location != ADJUDICATOR_NO_ERROR) { break; }
    }

    return error_location;
}

int MapAndUnits::process_provinces(const TokenMessage &provinces) {
    int error_location {ADJUDICATOR_NO_ERROR};
    TokenMessage supply_centres {};
    TokenMessage non_supply_centres {};

    // Resetting all provinces
    for (auto &province : game_map) {
        province.province_in_use = false;
        province.is_supply_centre = false;
        province.is_land = false;
        province.coast_info.clear();
        province.home_centre_set.clear();
    }

    // Retrieving SC and Non-SC info
    supply_centres = provinces.get_submessage(0);
    non_supply_centres = provinces.get_submessage(1);

    // Processing SC
    error_location = process_supply_centres(supply_centres);
    if (error_location != ADJUDICATOR_NO_ERROR) { return error_location + provinces.get_submessage_start(0); }

    // Processing Non-SC
    error_location = process_non_supply_centres(non_supply_centres);
    if (error_location != ADJUDICATOR_NO_ERROR) { return error_location + provinces.get_submessage_start(1); }

    // Counting provinces
    // MAX_PROVINCES has 256 provinces, while the standard map has only 81
    // We should not reach any used province after finding the first unused province
    number_of_provinces = -1;
    for (int province_ctr = 0; province_ctr < MAX_PROVINCES; province_ctr++) {

        // Error - Found a used province after an unused one
        if (game_map[province_ctr].province_in_use && number_of_provinces != -1) {
            return provinces.get_submessage_start(1) - 1;
        }

        // End of provinces - First unused province detected
        if (!game_map[province_ctr].province_in_use && number_of_provinces == -1) {
            number_of_provinces = province_ctr;
        }
    }

    return error_location;
}

int MapAndUnits::process_supply_centres(const TokenMessage &supply_centres) {
    int error_location {ADJUDICATOR_NO_ERROR};

    for (int submessage_ctr = 0; submessage_ctr < supply_centres.get_submessage_count(); submessage_ctr++) {
        error_location = process_supply_centres_for_power(supply_centres.get_submessage(submessage_ctr));
        if (error_location != ADJUDICATOR_NO_ERROR) {
            return error_location + supply_centres.get_submessage_start(submessage_ctr);
        }
    }
    return error_location;
}

int MapAndUnits::process_supply_centres_for_power(const TokenMessage &supply_centres) {
    int error_location {ADJUDICATOR_NO_ERROR};
    int province_index {-1};
    Token token {};
    Token power {TOKEN_PARAMETER_UNO};
    TokenMessage submessage {};
    HOME_CENTRE_SET home_centre_set;

    for (int submessage_ctr = 0; submessage_ctr < supply_centres.get_submessage_count(); submessage_ctr++) {
        submessage = supply_centres.get_submessage(submessage_ctr);

        // Single token (POWER, PROVINCE)
        if (submessage.is_single_token()) {
            token = submessage.get_token();

            // Token is power
            if (token.get_category() == CATEGORY_POWER) {

                // Error - Too many powers
                if (token.get_subtoken() >= number_of_powers) {
                    return supply_centres.get_submessage_start(submessage_ctr);
                }

                home_centre_set.insert(token.get_subtoken());
                power = token;

            // Token is province
            } else if (token.is_province()) {
                province_index = token.get_subtoken();

                // Error - Province already in use
                if (game_map[province_index].province_in_use) {
                    return supply_centres.get_submessage_start(submessage_ctr);
                }

                game_map[province_index].province_token = token;
                game_map[province_index].province_in_use = true;
                game_map[province_index].home_centre_set = home_centre_set;
                game_map[province_index].is_supply_centre = true;
                game_map[province_index].owner = power;


            // Unexpected token
            } else if (token != TOKEN_PARAMETER_UNO) {
                return supply_centres.get_submessage_start(submessage_ctr);
            }

        // Multiple tokens
        } else {
            for (int token_ctr = 0; token_ctr < submessage.get_message_length(); token_ctr++) {
                token = submessage.get_token(token_ctr);

                // Power
                if (token.get_category() == CATEGORY_POWER) {

                    // Error - Too many powers
                    if (token.get_subtoken() >= number_of_powers) {
                        return token_ctr + supply_centres.get_submessage_start(submessage_ctr);
                    }

                    home_centre_set.insert(token.get_subtoken());
                    power = token;

                // Unknown token
                } else {
                    return supply_centres.get_submessage_start(submessage_ctr);
                }
            }
        }
    }

    return error_location;
}

int MapAndUnits::process_non_supply_centres(const TokenMessage &non_supply_centres) {
    int error_location {ADJUDICATOR_NO_ERROR};
    int province_index {-1};
    Token token {};

    for (int token_ctr = 0; token_ctr < non_supply_centres.get_message_length(); token_ctr++) {
        token = non_supply_centres.get_token(token_ctr);

        // Province
        if (token.is_province()) {
            province_index = token.get_subtoken();
            if (game_map[province_index].province_in_use) { return token_ctr; }
            game_map[province_index].province_token = token;
            game_map[province_index].province_in_use = true;
            game_map[province_index].owner = TOKEN_PARAMETER_UNO;

        // Unexpected token
        } else if (token != TOKEN_PARAMETER_UNO) { return token_ctr; }
    }

    return error_location;
}

int MapAndUnits::process_adjacencies(const TokenMessage &adjacencies) {
    int error_location {ADJUDICATOR_NO_ERROR};
    TokenMessage province_adjacency {};

    for (int province_ctr = 0; province_ctr < adjacencies.get_submessage_count(); province_ctr++) {
        province_adjacency = adjacencies.get_submessage(province_ctr);
        error_location = process_province_adjacency(province_adjacency);
        if (error_location != ADJUDICATOR_NO_ERROR) {
            return error_location + adjacencies.get_submessage_start(province_ctr);
        }
    }
    return error_location;
}

int MapAndUnits::process_province_adjacency(const TokenMessage &province_adjacency) {
    int error_location {ADJUDICATOR_NO_ERROR};
    Token province_token {};
    TokenMessage adjacency_list {};
    PROVINCE_DETAILS *province_details {nullptr};

    province_token = province_adjacency.get_token(0);
    province_details = &(game_map[province_token.get_subtoken()]);

    if (!province_details->province_in_use || !province_details->coast_info.empty()) {
        return 0;           // error_location = 0
    }

    for (int adjacency_ctr = 1; adjacency_ctr < province_adjacency.get_submessage_count(); adjacency_ctr++) {
        adjacency_list = province_adjacency.get_submessage(adjacency_ctr);
        error_location = process_adjacency_list(province_details, adjacency_list);
        if (error_location != ADJUDICATOR_NO_ERROR) {
            return error_location + province_adjacency.get_submessage_start(adjacency_ctr);
        }
    }
    return error_location;
}

int MapAndUnits::process_adjacency_list(PROVINCE_DETAILS *province_details, const TokenMessage &adjacency_list) {
    int error_location {ADJUDICATOR_NO_ERROR};
    COAST_ID adjacent_coast {-1};
    Token coast_token {};
    Token adjacent_coast_token {};
    Token province_token {};
    TokenMessage adjacency_token {};
    COAST_DETAILS *coast_details {nullptr};

    adjacency_token = adjacency_list.get_submessage(0);

    if (adjacency_token.is_single_token()) {
        coast_token = adjacency_token.get_token();
        adjacent_coast_token = coast_token;
        if (coast_token == TOKEN_UNIT_AMY) { province_details->is_land = true; }
    } else {
        coast_token = adjacency_token.get_token(1);
        adjacent_coast_token = TOKEN_UNIT_FLT;
    }

    coast_details = &(province_details->coast_info[coast_token]);
    if (!coast_details->adjacent_coasts.empty()) {
        return 0;               // error_location = 0
    }

    for (int adjacency_ctr = 1; adjacency_ctr < adjacency_list.get_submessage_count(); adjacency_ctr++) {
        adjacency_token = adjacency_list.get_submessage(adjacency_ctr);

        if (adjacency_token.is_single_token()) {
            province_token = adjacency_token.get_token();
            adjacent_coast.province_index = province_token.get_subtoken();
            adjacent_coast.coast_token = adjacent_coast_token;
        } else {
            province_token = adjacency_token.get_token(0);
            coast_token = adjacency_token.get_token(1);
            adjacent_coast.province_index = province_token.get_subtoken();
            adjacent_coast.coast_token = coast_token;
        }

        coast_details->adjacent_coasts.insert(adjacent_coast);
    }
    return error_location;
}

void MapAndUnits::set_power_played(const Token &power) {
    int power_index;                        // Power index for this power

    power_played = power;                   // Store the power played
    home_centres.clear();                   // Build the list of home centres

    if (power.get_category() == CATEGORY_POWER) {
        power_index = power.get_subtoken();

        for (int prov_ctr = 0; prov_ctr < number_of_provinces; prov_ctr++) {
            if (game_map[prov_ctr].home_centre_set.find(power_index) != game_map[prov_ctr].home_centre_set.end()) {
                home_centres.insert(prov_ctr);
            }
        }
    }
    game_started = true;                    // The game has started
}

int MapAndUnits::set_ownership(const TokenMessage &sco_message) {
    int error_location {ADJUDICATOR_NO_ERROR};
    TokenMessage sco_command {};
    TokenMessage sco_for_power {};

    // Check we have a map
    if (number_of_provinces != NO_MAP) {
        sco_command = sco_message.get_submessage(0);

        if (!sco_command.is_single_token() || (sco_command.get_token() != TOKEN_COMMAND_SCO)) {
            return 0;           // error_location = 0
        }

        // Resetting ownership
        our_centres.clear();

        for (int submessage_ctr = 1; submessage_ctr < sco_message.get_submessage_count(); submessage_ctr++) {
            sco_for_power = sco_message.get_submessage(submessage_ctr);
            error_location = process_sco_for_power(sco_for_power);
            if (error_location != ADJUDICATOR_NO_ERROR) {
                return error_location + sco_message.get_submessage_start(submessage_ctr);
            }
        }
    }
    return error_location;
}

int MapAndUnits::process_sco_for_power(const TokenMessage &sco_for_power) {
    int error_location {ADJUDICATOR_NO_ERROR};
    Token power {};
    Token province {};

    power = sco_for_power.get_token(0);             // The power owning the SCs

    for (int province_ctr = 1; province_ctr < sco_for_power.get_message_length(); province_ctr++) {
        province = sco_for_power.get_token(province_ctr);

        // Error - Too many provinces
        if (province.get_subtoken() >= number_of_provinces) { return province_ctr; }

        game_map[province.get_subtoken()].owner = power;
        if (power == power_played) { our_centres.insert(province.get_subtoken()); }
    }
    return error_location;
}

int MapAndUnits::set_units(const TokenMessage &now_message) {
    int error_location {ADJUDICATOR_NO_ERROR};
    TokenMessage now_command {};
    TokenMessage turn_message {};
    TokenMessage unit {};

    // Check we have a map
    if (number_of_provinces != NO_MAP) {
        now_command = now_message.get_submessage(0);

        if (!now_command.is_single_token() || (now_command.get_token() != TOKEN_COMMAND_NOW)) {
            return 0;               // error_location = 0
        }

        turn_message = now_message.get_submessage(1);
        current_season = turn_message.get_token(0);
        current_year = turn_message.get_token(1).get_number();

        // Resetting units
        units.clear();
        dislodged_units.clear();
        our_units.clear();
        our_dislodged_units.clear();
        open_home_centres.clear();
        our_winter_orders.builds_or_disbands.clear();
        our_winter_orders.number_of_waives = 0;

        // Store the unit positions
        for (int unit_ctr = 2; unit_ctr < now_message.get_submessage_count(); unit_ctr++) {
            unit = now_message.get_submessage(unit_ctr);
            error_location = process_now_unit(unit);
            if (error_location != ADJUDICATOR_NO_ERROR) {
                return error_location + now_message.get_submessage_start(unit_ctr);
            }
        }

        // Build the list of open home centres
        if (power_played != Token(0)) {
            open_home_centres.clear();

            for (int home_centre : home_centres) {
                if ((game_map[home_centre].owner == power_played) && (units.find(home_centre) == units.end())) {
                    open_home_centres.insert(home_centre);
                }
            }
            number_of_disbands = our_units.size() - our_centres.size();
        }
    }
    return error_location;
}

int MapAndUnits::process_now_unit(const TokenMessage &unit_message) {
    const int LEN_DISLODGED_UNIT_MSG = 5;
    int error_location {ADJUDICATOR_NO_ERROR};
    POWER_INDEX nationality {-1};
    Token unit_type {};
    Token prov {};
    Token coast {};
    TokenMessage location {};
    TokenMessage retreat_option_list {};
    UNIT_AND_ORDER unit;

    nationality = unit_message.get_token(0).get_subtoken();

    if (nationality >= number_of_powers) {
        return 0;               // error_location = 0
    }

    unit_type = unit_message.get_token(1);
    location = unit_message.get_submessage(2);

    // Getting province and coast
    if (location.is_single_token()) {
        prov = location.get_token();
        coast = unit_type;
    } else {
        if (unit_type != TOKEN_UNIT_FLT) { return 2; }
        prov = location.get_token(0);
        coast = location.get_token(1);
    }

    // Error - Too many provinces, or coast not found
    if (prov.get_subtoken() >= number_of_provinces) { return 2; }
    if (game_map[prov.get_subtoken()].coast_info.find(coast) == game_map[prov.get_subtoken()].coast_info.end()) {
        return 2;
    }

    // Setting unit
    unit.coast_id.province_index = prov.get_subtoken();
    unit.coast_id.coast_token = coast;
    unit.nationality = nationality;
    unit.unit_type = unit_type;
    unit.order_type = NO_ORDER;

    unit.no_convoy = false;
    unit.no_army_to_convoy = false;
    unit.convoy_broken = false;
    unit.support_void = false;
    unit.support_cut = false;
    unit.bounce = false;
    unit.dislodged = false;
    unit.unit_moves = true;
    unit.illegal_order = false;

    // Unit is dislodged
    if (unit_message.get_submessage_count() == LEN_DISLODGED_UNIT_MSG) {

        // Error - No retreat locs
        if (unit_message.get_submessage(3).get_token() != TOKEN_PARAMETER_MRT) {
            return unit_message.get_submessage_start(3);
        }

        // Setting retreat locs
        retreat_option_list = unit_message.get_submessage(4);
        unit.retreat_options.clear();

        for (int retreat_loc_ctr = 0; retreat_loc_ctr < retreat_option_list.get_submessage_count(); retreat_loc_ctr++) {
            unit.retreat_options.insert(get_coast_id(retreat_option_list.get_submessage(retreat_loc_ctr), unit_type));
        }
        dislodged_units[prov.get_subtoken()] = unit;
        if (nationality == power_played.get_subtoken()) {
            our_dislodged_units.insert(prov.get_subtoken());
        }

    // Unit is not dislodged
    } else {
        units[prov.get_subtoken()] = unit;
        if (nationality == power_played.get_subtoken()) {
            our_units.insert(prov.get_subtoken());
        }
    }
    return error_location;
}

int MapAndUnits::store_result(const TokenMessage &ord_message) {
    int error_location {ADJUDICATOR_NO_ERROR};      // The location of any error in the message
    POWER_INDEX power {-1};                         // The power which owns the order
    Token season {};                                // The season from the message
    Token order_type {};                            // The token indicating the order type
    TokenMessage turn {};                           // The turn from the message
    TokenMessage order {};                          // The order from the message
    TokenMessage result {};                         // The result from the message
    TokenMessage unit {};                           // The unit in the orders
    WINTER_ORDERS_FOR_POWER new_adj_orders {};      // A set of adjustment orders for a power
    UNIT_AND_ORDER new_unit;                        // A new unit to add to the results

    // No map set up yet, so can't process
    // Otherwise, invalid format
    if (number_of_provinces == NO_MAP) { return error_location; }
    if (ord_message.get_submessage_count() != 4) { return 0; }                          // error_location = 0
    if (!ord_message.get_submessage(0).is_single_token()) { return 0;}                  // error_location = 0
    if (ord_message.get_submessage(0).get_token() != TOKEN_COMMAND_ORD) { return 0; }   // error_location = 0

    // Getting results
    turn = ord_message.get_submessage(1);
    order = ord_message.get_submessage(2);
    result = ord_message.get_submessage(3);
    season = turn.get_token(0);

    // If it is the first spring or fall movement result, then clear all the previous results
    if (((season == TOKEN_SEASON_SPR) || (season == TOKEN_SEASON_FAL)) && (season != last_movement_result_season)) {
        last_movement_result_season = season;
        last_movement_results.clear();
        last_retreat_results.clear();
        last_adjustment_results.clear();
    }

    // Decode the order
    unit = order.get_submessage(0);
    order_type = order.get_submessage(1).get_token();
    power = unit.get_token(0).get_subtoken();

    // If it is a winter season
    if (season == TOKEN_SEASON_WIN) {
        // Find the adjustment orders for this power
        auto adj_orders_itr = last_adjustment_results.find(power);

        // If they don't exist, then create them
        if (adj_orders_itr == last_adjustment_results.end()) {
            new_adj_orders.builds_or_disbands.clear();
            new_adj_orders.number_of_waives = 0;
            adj_orders_itr = last_adjustment_results.insert(WINTER_ORDERS::value_type(power, new_adj_orders)).first;
        }

        // Add it to the adjustment results
        if (order_type == TOKEN_ORDER_WVE) {
            adj_orders_itr->second.number_of_waives++;
        } else {
            adj_orders_itr->second.builds_or_disbands.insert(
                    BUILDS_OR_DISBANDS::value_type(get_coast_from_unit(unit), TOKEN_RESULT_SUC));
            adj_orders_itr->second.is_building = (order_type == TOKEN_ORDER_BLD);
        }

    // Not Winter
    } else {
        // Set up the new unit
        new_unit.coast_id = get_coast_from_unit(unit);
        new_unit.nationality = power;
        new_unit.unit_type = unit.get_token(1);

        // Decode the order and result
        decode_order(new_unit, order);
        decode_result(new_unit, result);

        // Insert it in the correct phase
        if ((season == TOKEN_SEASON_SPR) || (season == TOKEN_SEASON_FAL)) {
            last_movement_results.insert(UNITS::value_type(new_unit.coast_id.province_index, new_unit));
        } else {
            last_retreat_results.insert(UNITS::value_type(new_unit.coast_id.province_index, new_unit));
        }
    }
    return error_location;
}

bool MapAndUnits::set_hold_order(PROVINCE_INDEX unit) {
    bool unit_ordered {true};                       // Whether the unit was ordered successfully

    auto unit_to_order = units.find(unit);
    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = HOLD_ORDER;
    }
    return unit_ordered;
}

bool MapAndUnits::set_move_order(PROVINCE_INDEX unit, const COAST_ID &destination) {
    bool unit_ordered {true};                       // Whether the unit was ordered successfully

    auto unit_to_order = units.find(unit);
    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = MOVE_ORDER;
        unit_to_order->second.move_dest = destination;
    }
    return unit_ordered;
}

bool MapAndUnits::set_support_to_hold_order(PROVINCE_INDEX unit, PROVINCE_INDEX supported_unit) {
    bool unit_ordered {true};                       // Whether the unit was ordered successfully

    auto unit_to_order = units.find(unit);
    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = SUPPORT_TO_HOLD_ORDER;
        unit_to_order->second.other_dest_province = supported_unit;
    }
    return unit_ordered;
}

bool MapAndUnits::set_support_to_move_order(PROVINCE_INDEX unit,
                                            PROVINCE_INDEX supported_unit,
                                            PROVINCE_INDEX destination) {
    bool unit_ordered {true};                       // Whether the unit was ordered successfully

    auto unit_to_order = units.find(unit);
    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = SUPPORT_TO_MOVE_ORDER;
        unit_to_order->second.other_source_province = supported_unit;
        unit_to_order->second.other_dest_province = destination;
    }
    return unit_ordered;
}

bool MapAndUnits::set_convoy_order(PROVINCE_INDEX unit,
                                   PROVINCE_INDEX convoyed_army,
                                   PROVINCE_INDEX destination) {
    bool unit_ordered {true};                       // Whether the unit was ordered successfully

    auto unit_to_order = units.find(unit);
    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = CONVOY_ORDER;
        unit_to_order->second.other_source_province = convoyed_army;
        unit_to_order->second.other_dest_province = destination;
    }
    return unit_ordered;
}

bool MapAndUnits::set_move_by_convoy_order(PROVINCE_INDEX unit,
                                           PROVINCE_INDEX destination,
                                           int number_of_steps,
                                           PROVINCE_INDEX step_list[]) {
    bool unit_ordered {true};                       // Whether the unit was ordered successfully

    auto unit_to_order = units.find(unit);
    if (unit_to_order == units.end()) {
        unit_ordered = false;

    } else {
        unit_to_order->second.order_type = MOVE_BY_CONVOY_ORDER;
        unit_to_order->second.move_dest.province_index = destination;
        unit_to_order->second.move_dest.coast_token = TOKEN_UNIT_AMY;
        unit_to_order->second.convoy_step_list.clear();

        for (int step_ctr = 0; step_ctr < number_of_steps; step_ctr++) {
            unit_to_order->second.convoy_step_list.push_back(step_list[step_ctr]);
        }
    }
    return unit_ordered;
}

bool MapAndUnits::set_disband_order(PROVINCE_INDEX unit) {
    bool unit_ordered {true};                       // Whether the unit was ordered successfully

    auto unit_to_order = dislodged_units.find(unit);
    if (unit_to_order == dislodged_units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = DISBAND_ORDER;
    }
    return unit_ordered;
}

bool MapAndUnits::set_retreat_order(PROVINCE_INDEX unit, const COAST_ID &destination) {
    bool unit_ordered {true};                       // Whether the unit was ordered successfully

    auto unit_to_order = dislodged_units.find(unit);
    if (unit_to_order == dislodged_units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = RETREAT_ORDER;
        unit_to_order->second.move_dest = destination;
    }
    return unit_ordered;
}

void MapAndUnits::set_build_order(const COAST_ID &location) {
    COAST_ID min_coast {};                          // Minimum coast for this province

    min_coast.province_index = location.province_index;
    min_coast.coast_token = Token(0, 0);

    auto matching_unit = our_winter_orders.builds_or_disbands.lower_bound(min_coast);
    if ((matching_unit != our_winter_orders.builds_or_disbands.end())
        && (matching_unit->first.province_index == location.province_index)) {

        our_winter_orders.builds_or_disbands.erase(matching_unit);
    }

    our_winter_orders.builds_or_disbands.insert(BUILDS_OR_DISBANDS::value_type(location, Token(0)));
    our_winter_orders.is_building = true;
}

bool MapAndUnits::set_remove_order(PROVINCE_INDEX unit) {
    bool unit_ordered {true};                       // Whether the unit was ordered successfully

    auto unit_itr = units.find(unit);
    if (unit_itr == units.end()) {
        unit_ordered = false;
    } else {
        our_winter_orders.builds_or_disbands.insert(
                BUILDS_OR_DISBANDS::value_type(unit_itr->second.coast_id, Token(0)));
        our_winter_orders.is_building = false;
    }
    return unit_ordered;
}

bool MapAndUnits::cancel_build_order(PROVINCE_INDEX location) {
    COAST_ID min_coast {};                              // Minimum coast for this province
    bool order_found {false};                           // Whether a matching order was found

    min_coast.province_index = location;
    min_coast.coast_token = Token(0, 0);

    auto matching_unit = our_winter_orders.builds_or_disbands.lower_bound(min_coast);
    if ((matching_unit != our_winter_orders.builds_or_disbands.end())
        && (matching_unit->first.province_index == location)) {

        our_winter_orders.builds_or_disbands.erase(matching_unit);
        order_found = true;
    } else {
        order_found = false;
    }
    return order_found;
}

bool MapAndUnits::any_orders_entered() {
    bool order_entered {false};                         // Whether any orders have been entered

    // Orders submitted for regular unit
    if ((current_season == TOKEN_SEASON_SPR) || (current_season == TOKEN_SEASON_FAL)) {
        for (auto &unit : units) {
            if (unit.second.order_type != NO_ORDER) { order_entered = true; }
        }

    // Orders submitted for dislodged unit
    } else if ((current_season == TOKEN_SEASON_SUM) || (current_season == TOKEN_SEASON_AUT)) {
        for (auto &dislodged_unit : dislodged_units) {
            if (dislodged_unit.second.order_type != NO_ORDER) { order_entered = true; }
        }

    // Builds, disbands or waive submitted
    } else {
        if (!our_winter_orders.builds_or_disbands.empty() || (our_winter_orders.number_of_waives != 0)) {
            order_entered = true;
        }
    }
    return order_entered;
}

DAIDE::TokenMessage MapAndUnits::build_sub_command() {
    TokenMessage sub_message {};                        // The sub message
    TokenMessage unit_order {};                         // The order for one unit

    // Building sub message
    sub_message = TOKEN_COMMAND_SUB;

    // Orders during movement phases
    if ((current_season == TOKEN_SEASON_SPR) || (current_season == TOKEN_SEASON_FAL)) {
        for (auto &unit : units) {
            if ((unit.second.nationality == power_played.get_subtoken()) && (unit.second.order_type != NO_ORDER)) {
                unit_order = describe_movement_order(&(unit.second));
                sub_message = sub_message & unit_order;
            }
        }


    // Orders during retreat phases
    } else if ((current_season == TOKEN_SEASON_SUM) || (current_season == TOKEN_SEASON_AUT)) {
        for (auto &d_unit : dislodged_units) {
            if ((d_unit.second.nationality == power_played.get_subtoken()) && (d_unit.second.order_type != NO_ORDER)) {
                unit_order = describe_retreat_order(&(d_unit.second));
                sub_message = sub_message & unit_order;
            }
        }

    // Orders during adjustment phases
    } else {
        for (auto &builds_or_disband : our_winter_orders.builds_or_disbands) {
            unit_order = power_played;

            if (builds_or_disband.first.coast_token == TOKEN_UNIT_AMY) {
                unit_order = unit_order + TOKEN_UNIT_AMY;
            } else {
                unit_order = unit_order + TOKEN_UNIT_FLT;
            }

            unit_order = unit_order + describe_coast(builds_or_disband.first);
            unit_order.enclose_this();

            if (our_winter_orders.is_building) {
                unit_order = unit_order + TOKEN_ORDER_BLD;
            } else {
                unit_order = unit_order + TOKEN_ORDER_REM;
            }

            sub_message = sub_message & unit_order;
        }

        // Waives
        for (int waive_ctr = 0; waive_ctr < our_winter_orders.number_of_waives; waive_ctr++) {
            unit_order = power_played + TOKEN_ORDER_WVE;
            sub_message = sub_message & unit_order;
        }
    }

    return sub_message;
}

void MapAndUnits::clear_all_orders() {
    for (auto &unit : units) { unit.second.order_type = NO_ORDER; }
    for (auto &dislodged_unit : dislodged_units) { dislodged_unit.second.order_type = NO_ORDER; }
    our_winter_orders.builds_or_disbands.clear();
    our_winter_orders.number_of_waives = 0;
}

MapAndUnits::COAST_ID MapAndUnits::get_coast_id(const TokenMessage &coast, const Token &unit_type) {
    COAST_ID coast_id {};
    Token province_token {};

    if (coast.is_single_token()) {
        province_token = coast.get_token();
        coast_id.province_index = province_token.get_subtoken();
        coast_id.coast_token = unit_type;
    } else {
        province_token = coast.get_token(0);
        coast_id.province_index = province_token.get_subtoken();
        coast_id.coast_token = coast.get_token(1);
    }
    return coast_id;
}

TokenMessage MapAndUnits::describe_movement_order(UNIT_AND_ORDER *unit) {
    TokenMessage order {};
    TokenMessage convoy_via {};

    switch (unit->order_type) {
        case NO_ORDER:
        case HOLD_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_HLD;
            break;

        case MOVE_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_MTO + describe_coast(unit->move_dest);
            break;

        case SUPPORT_TO_HOLD_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_SUP + describe_unit(&(units[unit->other_dest_province]));
            break;

        case SUPPORT_TO_MOVE_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_SUP + describe_unit(&(units[unit->other_source_province]))
                    + TOKEN_ORDER_MTO + game_map[unit->other_dest_province].province_token;
            break;

        case CONVOY_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_CVY + describe_unit(&(units[unit->other_source_province]))
                    + TOKEN_ORDER_CTO + game_map[unit->other_dest_province].province_token;
            break;

        case MOVE_BY_CONVOY_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_CTO + describe_coast(unit->move_dest);
            for (int &convoy_step_itr : unit->convoy_step_list) {
                convoy_via = convoy_via + game_map[convoy_step_itr].province_token;
            }
            order = order + TOKEN_ORDER_VIA & convoy_via;
            break;

        default:
            break;
    }
    return order;
}

TokenMessage MapAndUnits::describe_coast(const COAST_ID &coast) {
    TokenMessage coast_message {};

    if (coast.coast_token.get_category() == CATEGORY_COAST) {
        coast_message = game_map[coast.province_index].province_token + coast.coast_token;
        coast_message.enclose_this();
    } else {
        coast_message = game_map[coast.province_index].province_token;
    }
    return coast_message;
}

TokenMessage MapAndUnits::describe_retreat_order(UNIT_AND_ORDER *unit) {
    TokenMessage order {};

    switch (unit->order_type) {
        case NO_ORDER:
        case DISBAND_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_DSB;
            break;

        case RETREAT_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_RTO + describe_coast(unit->move_dest);
            break;

        default:
            break;
    }
    return order;
}

MapAndUnits::COAST_ID MapAndUnits::get_coast_from_unit(const TokenMessage &unit) {
    return get_coast_id(unit.get_submessage(2), unit.get_submessage(1).get_token());
}

void MapAndUnits::decode_order(UNIT_AND_ORDER &unit, const TokenMessage &order) {
    Token order_type {};                                // The type of order
    TokenMessage convoy_step_list {};                   // The list of steps a convoy goes through

    order_type = order.get_submessage(1).get_token();

    // FIXME Use switch statement instead
    // Hold
    if (order_type == TOKEN_ORDER_HLD) {
        unit.order_type = HOLD_ORDER;

    // Move
    } else if (order_type == TOKEN_ORDER_MTO) {
        unit.order_type = MOVE_ORDER;
        unit.move_dest = get_coast_id(order.get_submessage(2), unit.unit_type);

    // Support
    } else if (order_type == TOKEN_ORDER_SUP) {
        if (order.get_submessage_count() == 3) {
            unit.order_type = SUPPORT_TO_HOLD_ORDER;
            unit.other_dest_province = get_coast_from_unit(order.get_submessage(2)).province_index;
        } else {
            unit.order_type = SUPPORT_TO_MOVE_ORDER;
            unit.other_source_province = get_coast_from_unit(order.get_submessage(2)).province_index;
            unit.other_dest_province = order.get_submessage(4).get_token().get_subtoken();
        }

    // Convoy order
    } else if (order_type == TOKEN_ORDER_CVY) {
        unit.order_type = CONVOY_ORDER;
        unit.other_source_province = get_coast_from_unit(order.get_submessage(2)).province_index;
        unit.other_dest_province = order.get_submessage(4).get_token().get_subtoken();

    // Move by convoy
    } else if (order_type == TOKEN_ORDER_CTO) {
        unit.order_type = MOVE_BY_CONVOY_ORDER;
        unit.move_dest = get_coast_id(order.get_submessage(2), unit.unit_type);
        unit.convoy_step_list.clear();
        convoy_step_list = order.get_submessage(4);
        for (int step_ctr = 0; step_ctr < convoy_step_list.get_message_length(); step_ctr++) {
            unit.convoy_step_list.push_back(convoy_step_list.get_token(step_ctr).get_subtoken());
        }

    // Disband
    } else if (order_type == TOKEN_ORDER_DSB) {
        unit.order_type = DISBAND_ORDER;

    // Retreat
    } else if (order_type == TOKEN_ORDER_RTO) {
        unit.order_type = RETREAT_ORDER;
        unit.move_dest = get_coast_id(order.get_submessage(2), unit.unit_type);
    }
}

void MapAndUnits::decode_result(UNIT_AND_ORDER &unit, const TokenMessage &result) {
    Token result_token {};                              // Token representing one part of the result

    unit.no_convoy = false;
    unit.no_army_to_convoy = false;
    unit.convoy_broken = false;
    unit.support_void = false;
    unit.support_cut = false;
    unit.bounce = false;
    unit.dislodged = false;
    unit.unit_moves = false;
    unit.illegal_order = false;

    for (int result_ctr = 0; result_ctr < result.get_message_length(); result_ctr++) {
        result_token = result.get_token(result_ctr);

        // Illegal order
        if (result_token.get_category() == CATEGORY_ORDER_NOTE) {
            unit.illegal_order = true;
            unit.illegal_reason = result_token;

        // Success
        } else if (result_token == TOKEN_RESULT_SUC) {
            if (unit.order_type == MOVE_ORDER) { unit.unit_moves = true; }
            if (unit.order_type == MOVE_BY_CONVOY_ORDER) { unit.unit_moves = true; }
            if (unit.order_type == RETREAT_ORDER) { unit.unit_moves = true; }

        // Bounce
        } else if (result_token == TOKEN_RESULT_BNC) {
            unit.bounce = true;

        // Cut
        } else if (result_token == TOKEN_RESULT_CUT) {
            unit.support_cut = true;

        // Disrupted
        } else if (result_token == TOKEN_RESULT_DSR) {
            unit.convoy_broken = true;

        // Void - No such order
        } else if (result_token == TOKEN_RESULT_NSO) {
            if ((unit.order_type == SUPPORT_TO_HOLD_ORDER) || (unit.order_type == SUPPORT_TO_MOVE_ORDER)) {
                unit.support_void = true;
            } else if (unit.order_type == CONVOY_ORDER) {
                unit.no_army_to_convoy = true;
            } else if (unit.order_type == MOVE_BY_CONVOY_ORDER) {
                unit.no_convoy = true;
            }

        // Dislodged
        } else if (result_token == TOKEN_RESULT_RET) {
            unit.dislodged = true;
        }
    }
}

std::string MapAndUnits::describe_movement_result(UNIT_AND_ORDER &unit) {
    std::string order;

    // Getting order
    order = describe_movement_order_string(unit, last_movement_results);

    // Adding suffix
    if (unit.bounce) { order = order + " (bounce)"; }
    if (unit.support_cut) { order = order + " (cut)"; }
    if ((unit.no_convoy) || (unit.no_army_to_convoy) || (unit.support_void)) { order = order + " (void)"; }
    if (unit.convoy_broken) { order = order + " (no convoy)"; }
    if (unit.illegal_order) { order = order + " (illegal)"; }
    if (unit.dislodged) { order = order + " (dislodged)"; }
    return order;
}

std::string MapAndUnits::describe_movement_order_string(UNIT_AND_ORDER &unit, UNITS &unit_set) {
    std::string order;
    std::string convoy_via;

    switch (unit.order_type) {
        case HOLD_ORDER:
            order = describe_unit_string(unit) + " Hold";
            break;

        case MOVE_ORDER:
            order = describe_unit_string(unit) + " - " + describe_coast_string(unit.move_dest);
            break;

        case SUPPORT_TO_HOLD_ORDER:
            order = describe_unit_string(unit) + " s " + describe_unit_string(unit_set[unit.other_dest_province]);
            break;

        case SUPPORT_TO_MOVE_ORDER:
            order = describe_unit_string(unit) + " s " + describe_unit_string(unit_set[unit.other_source_province])
                    + " - " + describe_province_string(unit.other_dest_province);

        case CONVOY_ORDER:
            order = describe_unit_string(unit) + " c " + describe_unit_string(unit_set[unit.other_source_province])
                    + " - " + describe_province_string(unit.other_dest_province);
            break;

        case MOVE_BY_CONVOY_ORDER:
            order = describe_unit_string(unit);
            for (int &convoy_step_itr : unit.convoy_step_list) {
                order += " - " + describe_province_string(convoy_step_itr);
            }
            order += " - " + describe_coast_string(unit.move_dest);
            break;

        default:
            break;
    }
    return order;
}

std::string MapAndUnits::describe_retreat_result(UNIT_AND_ORDER &unit) {
    std::string order;
    order = describe_retreat_order_string(unit);
    if (unit.bounce) { order = order + "(bounce)"; }
    return order;
}

std::string MapAndUnits::describe_retreat_order_string(UNIT_AND_ORDER &unit) {
    std::string order;

    switch (unit.order_type) {
        case DISBAND_ORDER:
            order = describe_unit_string(unit) + " Disband";
            break;

        case RETREAT_ORDER:
            order = describe_unit_string(unit) + " - " + describe_coast_string(unit.move_dest);
            break;

        default:
            break;
    }
    return order;
}

// Also works with maps...
template<class SetType>
typename SetType::value_type get_from_set(SetType my_set, int item_index) {
    auto set_itr = my_set.begin();
    for (int item_ctr = 0; item_ctr < item_index; item_ctr++) { set_itr++; }
    return *set_itr;
}

std::string MapAndUnits::describe_adjustment_result(WINTER_ORDERS_FOR_POWER &orders, int order_index) {
    std::string order;                              // The order description
    COAST_ID build_order;                           // The build order to describe

    if (order_index < orders.builds_or_disbands.size()) {
        order = orders.is_building ? "Build" : "Remove";
        build_order = get_from_set<BUILDS_OR_DISBANDS>(orders.builds_or_disbands, order_index).first;
        order += build_order.coast_token == TOKEN_UNIT_AMY ? " A(" : " F(";
        order += order + describe_coast_string(build_order) + ")";
    } else {
        order = "Waive";
    }
    return order;
}

int MapAndUnits::get_number_of_results(WINTER_ORDERS_FOR_POWER &orders) {
    return (orders.builds_or_disbands.size() + orders.number_of_waives);
}

std::string MapAndUnits::describe_unit_string(UNIT_AND_ORDER &unit) {
    std::string unit_string;
    unit_string = unit.unit_type == TOKEN_UNIT_AMY ? "A(" : "F(";
    unit_string += describe_coast_string(unit.coast_id) + ")";
    return unit_string;
}

std::string MapAndUnits::describe_coast_string(COAST_ID &coast) {
    std::string coast_string;
    coast_string = describe_province_string(coast.province_index);
    if (coast.coast_token == TOKEN_COAST_NCS) { coast_string += "/nc"; }
    if (coast.coast_token == TOKEN_COAST_SCS) { coast_string += "/sc"; }
    if (coast.coast_token == TOKEN_COAST_ECS) { coast_string += "/ec"; }
    if (coast.coast_token == TOKEN_COAST_WCS) { coast_string += "/wc"; }
    return coast_string;
}

std::string MapAndUnits::describe_province_string(PROVINCE_INDEX province_index) {
    TokenMessage province_token;                    // The token representing this province
    std::string province_upper;                    // The province abbreviation in upper case
    std::string province_abbrv;                    // The province abbreviation

    province_token = game_map[province_index].province_token;
    province_upper = province_token.get_message_as_text();

    if (game_map[province_index].is_land) {
        province_abbrv = province_upper[0];
        for (int char_ctr = 1; char_ctr < province_upper.size(); char_ctr++) {
            if (province_upper[char_ctr] == ' ') { continue; }
            province_abbrv += static_cast<char>(tolower(province_upper[char_ctr]));
        }
    } else {
        if (province_upper[province_upper.size() - 1] == ' ') {
            province_abbrv = province_upper.substr(0, province_upper.size() - 1);
        } else {
            province_abbrv = province_upper;
        }
    }
    return province_abbrv;
}

MapAndUnits::COAST_ID MapAndUnits::find_result_unit_initial_location(MapAndUnits::PROVINCE_INDEX province_index,
                                                                     bool &is_new_build,
                                                                     bool &retreated_to_province,
                                                                     bool &moved_to_province,
                                                                     bool &unit_found) {
    COAST_ID initial_location;                          // The initial location of the unit

    // Resetting
    is_new_build = false;
    retreated_to_province = false;
    moved_to_province = false;
    unit_found = false;

    // Builds
    for (auto &last_adjustment_result : last_adjustment_results) {
        if (last_adjustment_result.second.is_building) {
            for (auto &builds_or_disband : last_adjustment_result.second.builds_or_disbands) {
                if (builds_or_disband.first.province_index == province_index) {
                    initial_location = builds_or_disband.first;
                    is_new_build = true;
                    unit_found = true;
                }
            }
        }
    }
    if (unit_found) { return initial_location; }


    // Retreats
    for (auto &last_retreat_result : last_retreat_results) {
        if ((last_retreat_result.second.move_dest.province_index == province_index)
                && (last_retreat_result.second.unit_moves)) {
            initial_location = last_retreat_result.second.coast_id;
            retreated_to_province = true;
            unit_found = true;
        }
    }
    if (unit_found) { return initial_location; }

    // Movements
    for (auto &last_movement_result : last_movement_results) {
        if ((last_movement_result.second.move_dest.province_index == province_index)
                && (last_movement_result.second.unit_moves)) {
            initial_location = last_movement_result.second.coast_id;
            moved_to_province = true;
            unit_found = true;

        } else if ((last_movement_result.second.coast_id.province_index == province_index)
                    && !last_movement_result.second.unit_moves
                    && !last_movement_result.second.dislodged) {
            initial_location = last_movement_result.second.coast_id;
            unit_found = true;
        }
    }
    return initial_location;
}

bool MapAndUnits::get_variant_setting(const Token &variant_option, Token *parameter) {
    bool variant_found {false};
    TokenMessage variant_submessage {};

    // For each variant option
    for (int submessage_ctr = 0; submessage_ctr < variant.get_submessage_count(); submessage_ctr++) {
        variant_submessage = variant.get_submessage(submessage_ctr);

        // If it is the right one
        if (variant_submessage.get_token() == variant_option) {
            variant_found = true;

            // Get the parameter
            if ((variant_submessage.get_message_length() > 1) && (parameter != nullptr)) {
                *parameter = variant_submessage.get_token(1);
            }
        }
    }
    return variant_found;
}

MapAndUnits::COAST_SET *MapAndUnits::get_adjacent_coasts(COAST_ID &coast) {
    return &(game_map[coast.province_index].coast_info[coast.coast_token].adjacent_coasts);
}

MapAndUnits::COAST_SET *MapAndUnits::get_adjacent_coasts(PROVINCE_INDEX &unit_location) {
    COAST_SET *adjacent_coasts {nullptr};

    auto unit_itr = units.find(unit_location);
    if (unit_itr != units.end()) {
        adjacent_coasts = get_adjacent_coasts(unit_itr->second.coast_id);
    }
    return adjacent_coasts;
}

MapAndUnits::COAST_SET *MapAndUnits::get_dislodged_unit_adjacent_coasts(PROVINCE_INDEX &dislodged_unit_location) {
    COAST_SET *adjacent_coasts {nullptr};

    auto unit_itr = dislodged_units.find(dislodged_unit_location);
    if (unit_itr != dislodged_units.end()) {
        adjacent_coasts = get_adjacent_coasts(unit_itr->second.coast_id);
    }
    return adjacent_coasts;
}

void MapAndUnits::set_order_checking(bool check_on_submit, bool check_on_adjudicate) {
    check_orders_on_submission = check_on_submit;
    check_orders_on_adjudication = check_on_adjudicate;
}

int MapAndUnits::process_orders(const TokenMessage &sub_message, POWER_INDEX power_index, Token *order_result) {
    int error_location {ADJUDICATOR_NO_ERROR};
    TokenMessage sub_command {};
    TokenMessage order {};

    sub_command = sub_message.get_submessage(0);

    // Error - Wrong syntax for SUB
    if (!sub_command.is_single_token()) { return 0; }                   // error_location is 0
    if (sub_command.get_token() != TOKEN_COMMAND_SUB) { return 0; }     // error_location is 0

    for (int submessage_ctr = 1; submessage_ctr < sub_message.get_submessage_count(); submessage_ctr++) {
        order = sub_message.get_submessage(submessage_ctr);
        order_result[submessage_ctr - 1] = process_order(order, power_index);
    }
    return error_location;
}

Token MapAndUnits::process_order(TokenMessage &order, POWER_INDEX power_index) {
    int previous_province {-1};
    int convoying_province {-1};
    Token order_result {TOKEN_ORDER_NOTE_MBV};
    Token order_token {};
    Token support_destination {};
    Token convoy_destination {};
    TokenMessage order_token_message {};
    TokenMessage convoy_via_list {};
    TokenMessage winter_order_unit {};
    UNIT_AND_ORDER *unit_record {nullptr};
    UNIT_AND_ORDER *supported_unit {nullptr};
    UNIT_AND_ORDER *convoyed_unit {nullptr};
    UNIT_AND_ORDER *convoying_unit {nullptr};
    WINTER_ORDERS_FOR_POWER *winter_record {nullptr};
    COAST_ID destination {};
    COAST_ID build_loc {};
    COAST_ID build_province {};

    // Getting order
    order_token_message = order.get_submessage(1);
    order_token = order_token_message.get_token();

    // Movement
    if ((order_token.get_token() & ORDER_TURN_MASK) == ORDER_MOVE_TURN_CHECK) {
        unit_record = find_unit(order.get_submessage(0), units);

        // NRS - Not Right Season
        // NSU - No such unit
        // NYU - Not your unit
        if (current_season != TOKEN_SEASON_SPR && current_season != TOKEN_SEASON_FAL) { return TOKEN_ORDER_NOTE_NRS; }
        if (unit_record == nullptr) { return TOKEN_ORDER_NOTE_NSU; }
        if (unit_record->nationality != power_index) { return TOKEN_ORDER_NOTE_NYU; }
    }

    // Retreat
    if ((order_token.get_token() & ORDER_TURN_MASK) == ORDER_RETREAT_TURN_CHECK) {
        unit_record = find_unit(order.get_submessage(0), dislodged_units);

        // NRS - Not Right Season
        // NRN - No retreat needed
        // NYU - Not your unit
        if (current_season != TOKEN_SEASON_SUM && current_season != TOKEN_SEASON_AUT) { return TOKEN_ORDER_NOTE_NRS; }
        if (unit_record == nullptr) { return TOKEN_ORDER_NOTE_NRN; }
        if (unit_record->nationality != power_index) { return TOKEN_ORDER_NOTE_NYU; }
    }

    // Adjustment
    if ((order_token.get_token() & ORDER_TURN_MASK) == ORDER_BUILD_TURN_CHECK) {
        if (current_season != TOKEN_SEASON_WIN) { return TOKEN_ORDER_NOTE_NRS; }
        winter_record = &(winter_orders[power_index]);
    }

    // Hold Order
    if (order_token == TOKEN_ORDER_HLD) {
        unit_record->order_type = HOLD_ORDER;
    }

    // Move Order
    if (order_token == TOKEN_ORDER_MTO) {
        destination = get_coast_id(order.get_submessage(2), unit_record->unit_type);

        // FAR - Not Adjacent
        if (check_orders_on_submission && !can_move_to(unit_record, destination)) { return TOKEN_ORDER_NOTE_FAR; }
        unit_record->order_type = MOVE_ORDER;
        unit_record->move_dest = destination;
    }

    // Support Order
    if (order_token == TOKEN_ORDER_SUP) {

        // Support hold
        if (order.get_submessage_count() == 3) {
            supported_unit = find_unit(order.get_submessage(2), units);

            // NSU - No such unit
            // FAR - Not Adjacent
            if (supported_unit == nullptr) { return TOKEN_ORDER_NOTE_NSU; }
            if (check_orders_on_submission
                    && !can_move_to_province(unit_record, supported_unit->coast_id.province_index)) {
                return TOKEN_ORDER_NOTE_FAR;
            }
            if (check_orders_on_submission
                    && (supported_unit->coast_id.province_index == unit_record->coast_id.province_index)) {
                return TOKEN_ORDER_NOTE_FAR;                    // Supports itself
            }

            unit_record->order_type = SUPPORT_TO_HOLD_ORDER;
            unit_record->other_source_province = supported_unit->coast_id.province_index;

        // Support Move
        } else {
            supported_unit = find_unit(order.get_submessage(2), units);
            support_destination = order.get_submessage(4).get_token();

            // NSU - No such unit
            // FAR - Not Adjacent
            if (supported_unit == nullptr) { return TOKEN_ORDER_NOTE_NSU; }
            if (check_orders_on_submission
                    && !has_route_to_province(supported_unit,
                                              support_destination.get_subtoken(),
                                              unit_record->coast_id.province_index)) {
                return TOKEN_ORDER_NOTE_FAR;
            }
            if (check_orders_on_submission
                    && !can_move_to_province(unit_record, support_destination.get_subtoken())) {
                return TOKEN_ORDER_NOTE_FAR;
            }
            if (check_orders_on_submission
                    && (supported_unit->coast_id.province_index == unit_record->coast_id.province_index)) {
                return TOKEN_ORDER_NOTE_FAR;                    // Supports itself
            }

            unit_record->order_type = SUPPORT_TO_MOVE_ORDER;
            unit_record->other_source_province = supported_unit->coast_id.province_index;
            unit_record->other_dest_province = support_destination.get_subtoken();
        }
    }

    // Convoy Order
    if (order_token == TOKEN_ORDER_CVY) {
        convoyed_unit = find_unit(order.get_submessage(2), units);
        convoy_destination = order.get_submessage(4).get_token();

        // NSU - No such unit
        // NSF - No such fleet
        // NAS - Not at sea
        // FAR - Not adjacent
        if (convoyed_unit == nullptr) { return TOKEN_ORDER_NOTE_NSU; }
        if (check_orders_on_submission && (unit_record->unit_type != TOKEN_UNIT_FLT)) { return TOKEN_ORDER_NOTE_NSF; }
        if (check_orders_on_submission && (game_map[unit_record->coast_id.province_index].is_land)) {
            return TOKEN_ORDER_NOTE_NAS;
        }
        if (check_orders_on_submission && (convoyed_unit->unit_type != TOKEN_UNIT_AMY)) { return TOKEN_ORDER_NOTE_NSA; }
        if (check_orders_on_submission
                && !has_route_to_province(convoyed_unit, convoy_destination.get_subtoken(), -1)) {
            return TOKEN_ORDER_NOTE_FAR;
        }

        unit_record->order_type = CONVOY_ORDER;
        unit_record->other_source_province = convoyed_unit->coast_id.province_index;
        unit_record->other_dest_province = convoy_destination.get_subtoken();
    }

    // Move via convoy
    if (order_token == TOKEN_ORDER_CTO) {

        // NAS - Not at sea
        if (check_orders_on_submission && (unit_record->unit_type != TOKEN_UNIT_AMY)) { return TOKEN_ORDER_NOTE_NSA; }

        convoy_destination = order.get_submessage(2).get_token();
        convoy_via_list = order.get_submessage(4);
        previous_province = unit_record->coast_id.province_index;

        if (check_orders_on_submission) {
            for (int step_ctr = 0; step_ctr < convoy_via_list.get_message_length(); step_ctr++) {
                auto convoying_unit_itr = units.find(convoy_via_list
                                                     .get_submessage(step_ctr)
                                                     .get_token()
                                                     .get_subtoken());
                if (convoying_unit_itr != units.end()) { convoying_unit = &(convoying_unit_itr->second); }

                // NSF - No such fleet
                // NAS - Not at sea
                // FAR - Not adjacent
                if (convoying_unit == nullptr) { return TOKEN_ORDER_NOTE_NSF; }
                if (game_map[convoying_unit->coast_id.province_index].is_land) { return TOKEN_ORDER_NOTE_NAS; }
                if (!can_move_to_province(convoying_unit, previous_province)) { return TOKEN_ORDER_NOTE_FAR; }

                previous_province = convoying_unit->coast_id.province_index;
            }

            // FAR - Not Adjacent
            if (convoy_destination.get_subtoken() == unit_record->coast_id.province_index) {
                return TOKEN_ORDER_NOTE_FAR;
            }
            if (!can_move_to_province(convoying_unit, convoy_destination.get_subtoken())) {
                return TOKEN_ORDER_NOTE_FAR;
            }
        }

        unit_record->order_type = MOVE_BY_CONVOY_ORDER;
        unit_record->move_dest.province_index = convoy_destination.get_subtoken();
        unit_record->move_dest.coast_token = TOKEN_UNIT_AMY;

        unit_record->convoy_step_list.clear();
        for (int step_ctr = 0; step_ctr < convoy_via_list.get_message_length(); step_ctr++) {
            convoying_province = convoy_via_list.get_token(step_ctr).get_subtoken();
            unit_record->convoy_step_list.push_back(convoying_province);
        }

    // Retreat order
    } else if (order_token == TOKEN_ORDER_RTO) {
        destination = get_coast_id(order.get_submessage(2), unit_record->unit_type);

        // FAR - Not Adjacent
        // NVR - Not a valid retreat space
        if (check_orders_on_submission && !can_move_to(unit_record, destination)) { return TOKEN_ORDER_NOTE_FAR; }
        if (check_orders_on_submission
                && (unit_record->retreat_options.find(destination) == unit_record->retreat_options.end())) {
            return TOKEN_ORDER_NOTE_NVR;
        }

        unit_record->move_dest = destination;
        unit_record->order_type = RETREAT_ORDER;

    // Disband Order
    } else if (order_token == TOKEN_ORDER_DSB) {
        unit_record->order_type = DISBAND_ORDER;

    // Build Order
    } else if (order_token == TOKEN_ORDER_BLD) {

        // NMB - No more builds allowed
        if (!winter_record->is_building || (winter_record->builds_or_disbands.size() + winter_record->number_of_waives
                                            >= winter_record->number_of_orders_required)) {
            return TOKEN_ORDER_NOTE_NMB;
        }

        // Getting build location
        winter_order_unit = order.get_submessage(0);
        if (winter_order_unit.submessage_is_single_token(2)) {
            build_loc.province_index = winter_order_unit.get_token(2).get_subtoken();
            build_loc.coast_token = winter_order_unit.get_token(1);
        } else {
            build_loc.province_index = winter_order_unit.get_submessage(2).get_token(0).get_subtoken();
            build_loc.coast_token = winter_order_unit.get_submessage(2).get_token(1);
        }

        // NYU - Not your unit
        // NSC - Not a supply centre
        // HSC - Not a home supply centre
        // YSC - Not your supply centre
        // ESC - Not empty supply centre
        // CST - Invalid coast - No coast specified
        if (winter_order_unit.get_token(0).get_subtoken() != power_index) { return TOKEN_ORDER_NOTE_NYU; }
        if (!game_map[build_loc.province_index].is_supply_centre) { return TOKEN_ORDER_NOTE_NSC; }
        if (game_map[build_loc.province_index].home_centre_set.find(power_index)
                   == game_map[build_loc.province_index].home_centre_set.end()) {
            return TOKEN_ORDER_NOTE_HSC;
        }
        if (game_map[build_loc.province_index].owner.get_subtoken() != power_index) { return TOKEN_ORDER_NOTE_YSC; }
        if (units.find(build_loc.province_index) != units.end()) { return TOKEN_ORDER_NOTE_ESC; }
        if (game_map[build_loc.province_index].coast_info.find(build_loc.coast_token)
                   == game_map[build_loc.province_index].coast_info.end()) {
            return TOKEN_ORDER_NOTE_CST;
        }

        // Building
        build_province.province_index = build_loc.province_index;
        build_province.coast_token = 0;

        auto match_itr = winter_record->builds_or_disbands.lower_bound(build_province);

        // ESC - Not empty supply centre
        if (check_orders_on_submission
                && (match_itr != winter_record->builds_or_disbands.end())
                && (match_itr->first.province_index == build_province.province_index)) {
            return TOKEN_ORDER_NOTE_ESC;
        }

         // Legal build
         winter_record->builds_or_disbands.insert(BUILDS_OR_DISBANDS::value_type(build_loc, TOKEN_ORDER_NOTE_MBV));

    // Remove Order
    } else if (order_token == TOKEN_ORDER_REM) {
        unit_record = find_unit(order.get_submessage(0), units);

        // NMR - No more removals allowed
        // NSU - No such unit
        // NYU - Not your unit
        if ((winter_record->is_building) || (winter_record->builds_or_disbands.size()
                                             >= winter_record->number_of_orders_required)) {
            return TOKEN_ORDER_NOTE_NMR;
        }
        if (unit_record == nullptr) { return TOKEN_ORDER_NOTE_NSU; }
        if (unit_record->nationality != power_index) { return TOKEN_ORDER_NOTE_NYU; }

        // Legal removal
        winter_record->builds_or_disbands.insert(
                BUILDS_OR_DISBANDS::value_type(unit_record->coast_id, TOKEN_ORDER_NOTE_MBV));

    // Waive Order
    } else if (order_token == TOKEN_ORDER_WVE) {

        // NMB - No more builds allowed
        // NYU - Not  your unit
        if (!winter_record->is_building || (winter_record->builds_or_disbands.size() + winter_record->number_of_waives
                                            >= winter_record->number_of_orders_required)) {
            return TOKEN_ORDER_NOTE_NMB;
        }
        if (order.get_token(0).get_subtoken() != power_index) { return TOKEN_ORDER_NOTE_NYU; }

        winter_record->number_of_waives++;
    }

    return order_result;
}

MapAndUnits::UNIT_AND_ORDER *MapAndUnits::find_unit(const TokenMessage &unit_to_find, UNITS &units_map) {
    UNIT_AND_ORDER *found_unit {nullptr};
    Token province_token {};
    Token coast {};
    TokenMessage nationality {};
    TokenMessage unit_type {};
    TokenMessage location {};

    // Error - Invalid unit syntax - Expected POWER UNIT_TYPE LOCATION
    if (unit_to_find.get_submessage_count() != 3) { return found_unit; }

    nationality = unit_to_find.get_submessage(0);
    unit_type = unit_to_find.get_submessage(1);
    location = unit_to_find.get_submessage(2);

    // Error - Invalid format for power or unit type
    if (!nationality.is_single_token() || !unit_type.is_single_token()) { return found_unit; }

    // Retrieving unit
    if (location.is_single_token()) {
        province_token = location.get_token();
        coast = unit_type.get_token();
    } else {
        province_token = location.get_token(0);
        coast = location.get_token(1);
    }
    auto unit_itr = units_map.find(province_token.get_subtoken());

    // Unit not found or doesn't match
    if (unit_itr == units_map.end()) { return found_unit; }
    if ((province_token.get_subtoken() >= number_of_provinces)
        || (unit_itr->second.coast_id.coast_token != coast)
        || (unit_itr->second.nationality != nationality.get_token().get_subtoken())
        || (unit_itr->second.unit_type != unit_type.get_token())) { return found_unit; }

    found_unit = &(unit_itr->second);
    return found_unit;
}

bool MapAndUnits::can_move_to(UNIT_AND_ORDER *unit, const COAST_ID &destination) {
    bool can_move {false};

    auto coast_details = game_map[unit->coast_id.province_index].coast_info.find(unit->coast_id.coast_token);
    if (coast_details != game_map[unit->coast_id.province_index].coast_info.end()) {
        if (coast_details->second.adjacent_coasts.find(destination) != coast_details->second.adjacent_coasts.end()) {
            can_move = true;
        }
    }
    return can_move;
}

bool MapAndUnits::can_move_to_province(UNIT_AND_ORDER *unit, int province_index) {
    bool can_move {false};
    COAST_ID min_coast {};
    COAST_SET *adjacencies {nullptr};

    auto coast_details = game_map[unit->coast_id.province_index].coast_info.find(unit->coast_id.coast_token);
    if (coast_details != game_map[unit->coast_id.province_index].coast_info.end()) {
        adjacencies = &(coast_details->second.adjacent_coasts);
        min_coast.province_index = province_index;
        min_coast.coast_token = 0;

        auto first_record = adjacencies->lower_bound(min_coast);
        if ((first_record != adjacencies->end()) && (first_record->province_index == province_index)) {
            can_move = true;
        }
    }
    return can_move;
}

bool MapAndUnits::has_route_to_province(UNIT_AND_ORDER *unit,
                                        PROVINCE_INDEX province_index,
                                        PROVINCE_INDEX province_to_avoid) {
    bool has_route {false};                                 // Whether there is a route
    PROVINCE_SET checked_provinces;                         // Provinces which have already been checked
    PROVINCE_SET provinces_to_check;                        // Provinces still to check
    PROVINCE_INDEX province_being_checked {-1};             // Province currently being processed

    // First check if it can move directly
    has_route = can_move_to_province(unit, province_index);

    // If not, check for convoy routes
    if (!has_route && (unit->unit_type == TOKEN_UNIT_AMY) && (game_map[province_index].is_land)) {
        checked_provinces.insert(unit->coast_id.province_index);        // Mark the source province as checked

        // Add all the adjacent provinces to the province to check list
        for (auto &coast_itr : game_map[unit->coast_id.province_index].coast_info) {
            for (const auto &adjacent_coast : coast_itr.second.adjacent_coasts) {
                provinces_to_check.insert(adjacent_coast.province_index);
            }
        }

        // If there is a province to avoid, then mark it as already checked. This will stop
        // any routes from going through it. This is used to stop a fleet supporting a convoyed
        // move that must be convoyed by that fleet
        if (province_to_avoid != -1) {
            checked_provinces.insert(province_to_avoid);
        }

        // While there are provinces to check
        while (!provinces_to_check.empty() && !has_route) {
            province_being_checked = *(provinces_to_check.begin());     // Get the first
            provinces_to_check.erase(provinces_to_check.begin());

            // Check we haven't done it already
            if (checked_provinces.find(province_being_checked) == checked_provinces.end()) {
                checked_provinces.insert(province_being_checked);       // Add it to those we've done

                // If it is a land province then check if it is the destination
                if (game_map[province_being_checked].is_land) {
                    if (province_being_checked == province_index) {
                        has_route = true;
                    }

                // Sea province, so check if occupied. If it is then add all adjacent provinces
                } else {
                    if (units.find(province_being_checked) != units.end()) {

                        // Add all the adjacent provinces to the province to check list
                        for (auto &coast_itr : game_map[province_being_checked].coast_info) {
                            for (const auto &adjacent_coast : coast_itr.second.adjacent_coasts) {
                                provinces_to_check.insert(adjacent_coast.province_index);
                            }
                        }
                    }
                }
            }
        }
    }
    return has_route;
}

int MapAndUnits::get_adjudication_results(TokenMessage ord_messages[]) {
    int number_of_results {0};
    if ((current_season == TOKEN_SEASON_SPR) || (current_season == TOKEN_SEASON_FAL)) {
        number_of_results = get_movement_results(ord_messages);
    } else if ((current_season == TOKEN_SEASON_SUM) || (current_season == TOKEN_SEASON_AUT)) {
        number_of_results = get_retreat_results(ord_messages);
    } else {
        number_of_results = get_adjustment_results(ord_messages);
    }
    return number_of_results;
}

int MapAndUnits::get_movement_results(TokenMessage ord_messages[]) {
    int unit_ctr {0};
    UNIT_AND_ORDER *unit {nullptr};

    for (auto &unit_itr : units) {
        unit = &(unit_itr.second);
        ord_messages[unit_ctr] = describe_movement_result(unit);
        unit_ctr++;
    }
    return unit_ctr;
}

TokenMessage MapAndUnits::describe_movement_result(UNIT_AND_ORDER *unit) {
    TokenMessage movement_result {};
    TokenMessage order {};
    TokenMessage result {};
    TokenMessage convoy_via {};

    // Getting movement result
    movement_result = TOKEN_COMMAND_ORD + describe_turn();

    switch (unit->order_type) {
        case NO_ORDER:
        case HOLD_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_HLD;
            if (!unit->dislodged) { result = TOKEN_RESULT_SUC; }
            break;

        case MOVE_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_MTO + describe_coast(unit->move_dest);
            if (unit->bounce) { result = TOKEN_RESULT_BNC; }
            else if (unit->illegal_order) { result = unit->illegal_reason; }
            else { result = TOKEN_RESULT_SUC; }
            break;

        case SUPPORT_TO_HOLD_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_SUP + describe_unit(&(units[unit->other_source_province]));
            if (unit->support_cut) { result = TOKEN_RESULT_CUT; }
            else if (unit->support_void) { result = TOKEN_RESULT_NSO; }
            else if (unit->illegal_order) { result = unit->illegal_reason; }
            else { result = TOKEN_RESULT_SUC; }
            break;

        case SUPPORT_TO_MOVE_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_SUP + describe_unit(&(units[unit->other_source_province]))
                    + TOKEN_ORDER_MTO + game_map[unit->other_dest_province].province_token;
            if (unit->support_cut) { result = TOKEN_RESULT_CUT; }
            else if (unit->support_void) { result = TOKEN_RESULT_NSO; }
            else if (unit->illegal_order) { result = unit->illegal_reason; }
            else { result = TOKEN_RESULT_SUC; }
            break;

        case CONVOY_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_CVY + describe_unit(&(units[unit->other_source_province]))
                    + TOKEN_ORDER_CTO + game_map[unit->other_dest_province].province_token;
            if (unit->no_army_to_convoy) { result = TOKEN_RESULT_NSO; }
            else if (unit->illegal_order) { result = unit->illegal_reason; }
            else if (!unit->dislodged) { result = TOKEN_RESULT_SUC; }
            break;

        case MOVE_BY_CONVOY_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_CTO + describe_coast(unit->move_dest);
            for (int &convoy_step_itr : unit->convoy_step_list) {
                convoy_via = convoy_via + game_map[convoy_step_itr].province_token;
            }
            order = order + TOKEN_ORDER_VIA & convoy_via;

            if (unit->no_convoy) { result = TOKEN_RESULT_NSO; }
            else if (unit->convoy_broken) { result = TOKEN_RESULT_DSR; }
            else if (unit->bounce) { result = TOKEN_RESULT_BNC; }
            else if (unit->illegal_order) { result = unit->illegal_reason; }
            else { result = TOKEN_RESULT_SUC; }
            break;

        default:
            break;
    }

    if (unit->dislodged) { result = result + TOKEN_RESULT_RET; }
    movement_result = movement_result & order & result;
    return movement_result;
}

TokenMessage MapAndUnits::describe_turn() {
    Token current_year_token {};
    TokenMessage current_turn {};
    current_year_token.set_number(current_year);
    current_turn = current_season + current_year_token;
    current_turn.enclose_this();
    return current_turn;
}

TokenMessage MapAndUnits::describe_unit(UNIT_AND_ORDER *unit) {
    TokenMessage unit_message {};
    unit_message = Token(CATEGORY_POWER, unit->nationality) + unit->unit_type + describe_coast(unit->coast_id);
    unit_message.enclose_this();
    return unit_message;
}

int MapAndUnits::get_retreat_results(TokenMessage ord_messages[]) {
    int unit_ctr {0};
    UNIT_AND_ORDER *unit {nullptr};

    for (auto &dislodged_unit : dislodged_units) {
        unit = &(dislodged_unit.second);
        ord_messages[unit_ctr] = describe_retreat_result(unit);
        unit_ctr++;
    }
    return unit_ctr;
}

TokenMessage MapAndUnits::describe_retreat_result(UNIT_AND_ORDER *unit) {
    TokenMessage retreat_result {};
    TokenMessage order {};
    TokenMessage result {};

    // Getting retreat results
    retreat_result = TOKEN_COMMAND_ORD + describe_turn();

    switch (unit->order_type) {
        case NO_ORDER:
        case DISBAND_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_DSB;
            result = TOKEN_RESULT_SUC;
            break;

        case RETREAT_ORDER:
            order = describe_unit(unit) + TOKEN_ORDER_RTO + describe_coast(unit->move_dest);
            if (unit->bounce) { result = TOKEN_RESULT_BNC; }
            else if (unit->illegal_order) { result = unit->illegal_reason; }
            else { result = TOKEN_RESULT_SUC; }
            break;

        default:
            break;
    }

    retreat_result = retreat_result & order & result;
    return retreat_result;
}

int MapAndUnits::get_adjustment_results(TokenMessage ord_messages[]) {
    int order_ctr {0};
    WINTER_ORDERS_FOR_POWER *orders {nullptr};

    for (POWER_INDEX power_ctr = 0; power_ctr < number_of_powers; power_ctr++) {
        orders = &(winter_orders[power_ctr]);

        for (auto order_itr = orders->builds_or_disbands.begin();
             order_itr != orders->builds_or_disbands.end();
             order_itr++) {

            ord_messages[order_ctr] = describe_build_result(power_ctr, orders, order_itr);
            order_ctr++;
        }

        if (orders->is_building) {
            for (int waive_ctr = 0; waive_ctr < orders->number_of_waives; waive_ctr++) {
                ord_messages[order_ctr] = describe_waive(power_ctr);
                order_ctr++;
            }
        }
    }
    return order_ctr;
}

TokenMessage MapAndUnits::describe_build_result(POWER_INDEX power_ctr,
                                                WINTER_ORDERS_FOR_POWER *orders,
                                                BUILDS_OR_DISBANDS::iterator order_itr) {
    TokenMessage build_result_message {};
    TokenMessage order {};

    // Building order
    order = Token(CATEGORY_POWER, power_ctr);
    order = order + (order_itr->first.coast_token == TOKEN_UNIT_AMY ? TOKEN_UNIT_AMY : TOKEN_UNIT_FLT);
    order = order + describe_coast(order_itr->first);
    order.enclose_this();
    order = order + (orders->is_building ? TOKEN_ORDER_BLD : TOKEN_ORDER_REM);

    build_result_message = TOKEN_COMMAND_ORD + describe_turn() & order & TOKEN_RESULT_SUC;
    return build_result_message;
}

TokenMessage MapAndUnits::describe_waive(POWER_INDEX power_ctr) {
    TokenMessage build_result_message {};
    TokenMessage order {};

    order = Token(CATEGORY_POWER, power_ctr) + TOKEN_ORDER_WVE;
    build_result_message = TOKEN_COMMAND_ORD + describe_turn() & order & TOKEN_RESULT_SUC;
    return build_result_message;
}

void MapAndUnits::get_unit_positions(TokenMessage *now_message) {
    *now_message = TOKEN_COMMAND_NOW + describe_turn();
    for (auto &unit : units) {
        *now_message = *now_message + describe_unit(&(unit.second));
    }
    for (auto &dislodged_unit : dislodged_units) {
        *now_message = *now_message + describe_dislodged_unit(&(dislodged_unit.second));
    }
}

TokenMessage MapAndUnits::describe_dislodged_unit(UNIT_AND_ORDER *unit) {
    TokenMessage unit_message {};
    TokenMessage retreat_locations {};

    unit_message = Token(CATEGORY_POWER, unit->nationality) + unit->unit_type + describe_coast(unit->coast_id)
                   + TOKEN_PARAMETER_MRT;
    for (const auto &retreat_option : unit->retreat_options) {
        retreat_locations + retreat_locations + describe_coast(retreat_option);
    }

    unit_message = (unit_message & retreat_locations).enclose();
    return unit_message;
}

void MapAndUnits::get_sc_ownerships(TokenMessage *sco_message) {
    TokenMessage power_owned_scs[MAX_POWERS] {};
    TokenMessage unowned_scs {};
    int sc_owner {-1};

    // Building a list of SC ownership
    for (int province_ctr = 0; province_ctr < number_of_provinces; province_ctr++) {
        if (game_map[province_ctr].is_supply_centre) {
            if (game_map[province_ctr].owner == TOKEN_PARAMETER_UNO) {
                unowned_scs = unowned_scs + game_map[province_ctr].province_token;
            } else {
                sc_owner = game_map[province_ctr].owner.get_subtoken();
                power_owned_scs[sc_owner] = power_owned_scs[sc_owner] + game_map[province_ctr].province_token;
            }
        }
    }

    // Building SCO command
    *sco_message = TOKEN_COMMAND_SCO;
    for (int power_ctr = 0; power_ctr < number_of_powers; power_ctr++) {
        if (power_owned_scs[power_ctr].get_message_length() != TokenMessage::NO_MESSAGE) {
            *sco_message = *sco_message & (Token(CATEGORY_POWER, power_ctr) + power_owned_scs[power_ctr]);
        }
    }
    if (unowned_scs.get_message_length() != TokenMessage::NO_MESSAGE) {
        *sco_message = *sco_message & (TOKEN_PARAMETER_UNO + unowned_scs);
    }
}

int MapAndUnits::get_centre_count(const Token &power) {
    int centre_count {0};

    for (int province_ctr = 0; province_ctr < number_of_provinces; province_ctr++) {
        if (game_map[province_ctr].is_supply_centre) {
            if (game_map[province_ctr].owner == power) {
                centre_count++;
            }
        }
    }
    return centre_count;
}

int MapAndUnits::get_unit_count(const Token &power) {
    int unit_count {0};
    for (auto &unit : units) {
        if (unit.second.nationality == power.get_subtoken()) { unit_count++; }
    }
    return unit_count;
}

bool MapAndUnits::check_if_all_orders_received(POWER_INDEX power_index) {

    // Go through all units and determine if any haven't been ordered
    if ((current_season == TOKEN_SEASON_SPR) || (current_season == TOKEN_SEASON_FAL)) {
        for (auto &unit : units) {
            if ((unit.second.nationality == power_index) && (unit.second.order_type == NO_ORDER)) {
                return false;
            }
        }
    }

    // Go through all dislodged units and determine if any haven't been ordered
    if ((current_season == TOKEN_SEASON_SUM) || (current_season == TOKEN_SEASON_AUT)) {
        for (auto &dislodged_unit : dislodged_units) {
            if ((dislodged_unit.second.nationality == power_index) && (dislodged_unit.second.order_type == NO_ORDER)) {
                return false;
            }
        }
    }

    // Find the record for this power
    if (current_season == TOKEN_SEASON_WIN) {
        auto winter_itr = winter_orders.find(power_index);
        if (winter_itr != winter_orders.end()) {
            if (winter_itr->second.number_of_orders_required > (winter_itr->second.builds_or_disbands.size()
                                                                + winter_itr->second.number_of_waives)) {
                return false;
            }
        }
    }

    return true;            // All units have been ordered
}

bool MapAndUnits::unorder_adjustment(const TokenMessage &not_sub_message, int power_index) {
    Token order_token {};
    TokenMessage sub_message {};
    TokenMessage order {};
    TokenMessage order_token_message {};
    TokenMessage winter_order_unit {};
    WINTER_ORDERS_FOR_POWER *winter_record {nullptr};
    COAST_ID build_location {};

    // Getting order
    sub_message = not_sub_message.get_submessage(1);
    order = sub_message.get_submessage(1);
    order_token_message = order.get_submessage(1);
    order_token = order_token_message.get_token();

    // Checking that order is a build and that we are in Adjustment phase
    if ((order_token.get_token() & ORDER_TURN_MASK) == ORDER_BUILD_TURN_CHECK) {
        if (current_season != TOKEN_SEASON_WIN) { return false; }
        winter_record = &(winter_orders[power_index]);
    } else { return false; }

    // Build or Disband
    if ((order_token == TOKEN_ORDER_BLD) || (order_token == TOKEN_ORDER_REM)) {
        if (winter_record->is_building ^ (order_token == TOKEN_ORDER_BLD)) { return false; }

        // Getting build / disband location
        winter_order_unit = order.get_submessage(0);
        if (winter_order_unit.submessage_is_single_token(2)) {
            build_location.province_index = winter_order_unit.get_token(2).get_subtoken();
            build_location.coast_token = winter_order_unit.get_token(1);
        } else {
            build_location.province_index = winter_order_unit.get_submessage(2).get_token(0).get_subtoken();
            build_location.coast_token = winter_order_unit.get_submessage(2).get_token(1);
        }

        // Checking if correct power provided
        if (winter_order_unit.get_token(0).get_subtoken() != power_index) { return false; }

        // Deleting order
        auto match_itr = winter_record->builds_or_disbands.find(build_location);
        if (match_itr == winter_record->builds_or_disbands.end()) { return false; }
        winter_record->builds_or_disbands.erase(match_itr);

    // Waive
    } else if (order_token == TOKEN_ORDER_WVE) {
        if (!winter_record->is_building || (winter_record->number_of_waives == 0)) { return false; }
        if (order.get_token(0).get_subtoken() != power_index) { return false; }
        winter_record->number_of_waives--;
    }

    return true;                // Order is valid and has been reverted
}
