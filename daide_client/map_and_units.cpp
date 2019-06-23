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

#include "stdafx.h"
#include "map_and_units.h"
#include "token_message.h"

// Function to get the instance of the MapAndUnits object
MapAndUnits *MapAndUnits::get_instance() {
    static MapAndUnits object_instance;

    return &object_instance;
}

// Get a duplicate copy of the MapAndUnits class. Recommended for messing around with
// without affecting the master copy
MapAndUnits *MapAndUnits::get_duplicate_instance() {
    MapAndUnits *original;
    MapAndUnits *duplicate;

    original = get_instance();

    duplicate = new MapAndUnits;

    *duplicate = *original;

    return duplicate;
}

// Delete a duplicate
void MapAndUnits::delete_duplicate_instance(MapAndUnits *duplicate) {
    delete duplicate;
}

// Private constructor. Use get_instance to get the object
MapAndUnits::MapAndUnits() {
    game_over = false;
    game_started = false;
    power_played = Token(0);
    number_of_provinces = NO_MAP;
    number_of_powers = NO_MAP;
}

int MapAndUnits::set_map(TokenMessage &mdf_message) {
    int error_location = ADJUDICATOR_NO_ERROR;
    // location of error in the message, or ADJUDICATOR_NO_ERROR
    TokenMessage mdf_command;            // The command
    TokenMessage power_list;            // The list of powers
    TokenMessage provinces;                // The provinces
    TokenMessage adjacencies;            // The adjacencies

    // Check there are 4 submessages
    if (mdf_message.get_submessage_count() != 4) {
        error_location = 0;
    } else {
        // Get the four parts of the message
        mdf_command = mdf_message.get_submessage(0);
        power_list = mdf_message.get_submessage(1);
        provinces = mdf_message.get_submessage(2);
        adjacencies = mdf_message.get_submessage(3);

        // Check the mdf command
        if (!mdf_command.is_single_token()
            || mdf_command.get_token() != TOKEN_COMMAND_MDF) {
            error_location = 0;
        }
    }

    if (error_location == ADJUDICATOR_NO_ERROR) {
        error_location = process_power_list(power_list);

        if (error_location != ADJUDICATOR_NO_ERROR) {
            error_location = error_location + mdf_message.get_submessage_start(1);
        }
    }

    if (error_location == ADJUDICATOR_NO_ERROR) {
        error_location = process_provinces(provinces);

        if (error_location != ADJUDICATOR_NO_ERROR) {
            error_location = error_location + mdf_message.get_submessage_start(2);
        }
    }

    if (error_location == ADJUDICATOR_NO_ERROR) {
        error_location = process_adjacencies(adjacencies);

        if (error_location != ADJUDICATOR_NO_ERROR) {
            error_location = error_location + mdf_message.get_submessage_start(3);
        }
    }

    return error_location;
}

int MapAndUnits::process_power_list(TokenMessage &power_list) {
    bool power_used[MAX_POWERS];
    int power_counter;
    Token power;
    int error_location = ADJUDICATOR_NO_ERROR;

    number_of_powers = power_list.get_message_length();

    for (power_counter = 0; power_counter < number_of_powers; power_counter++) {
        power_used[power_counter] = false;
    }

    for (power_counter = 0;
         (power_counter < number_of_powers) && (error_location == ADJUDICATOR_NO_ERROR);
         power_counter++) {
        power = power_list.get_token(power_counter);

        if ((power.get_subtoken() >= number_of_powers)
            || (power_used[power.get_subtoken()])) {
            error_location = power_counter;
        } else {
            power_used[power.get_subtoken()] = true;
        }
    }

    return error_location;
}

int MapAndUnits::process_provinces(TokenMessage &provinces) {
    int error_location = ADJUDICATOR_NO_ERROR;
    int province_counter;
    TokenMessage supply_centres;
    TokenMessage non_supply_centres;

    for (province_counter = 0; province_counter < MAX_PROVINCES; province_counter++) {
        game_map[province_counter].province_in_use = false;
        game_map[province_counter].is_supply_centre = false;
        game_map[province_counter].is_land = false;
        game_map[province_counter].coast_info.clear();
        game_map[province_counter].home_centre_set.clear();
    }

    supply_centres = provinces.get_submessage(0);
    non_supply_centres = provinces.get_submessage(1);

    error_location = process_supply_centres(supply_centres);

    if (error_location != ADJUDICATOR_NO_ERROR) {
        error_location = error_location + provinces.get_submessage_start(0);
    } else {
        error_location = process_non_supply_centres(non_supply_centres);

        if (error_location != ADJUDICATOR_NO_ERROR) {
            error_location = error_location + provinces.get_submessage_start(1);
        }
    }

    if (error_location == ADJUDICATOR_NO_ERROR) {
        number_of_provinces = -1;

        for (province_counter = 0; province_counter < MAX_PROVINCES; province_counter++) {
            if (!game_map[province_counter].province_in_use) {
                if (number_of_provinces == -1) {
                    number_of_provinces = province_counter;
                }
            } else {
                if (number_of_provinces != -1) {
                    error_location = provinces.get_submessage_start(1) - 1;
                }
            }
        }
    }

    return error_location;
}

int MapAndUnits::process_supply_centres(TokenMessage &supply_centres) {
    int error_location = ADJUDICATOR_NO_ERROR;
    int submessage_counter;

    for (submessage_counter = 0;
         (submessage_counter < supply_centres.get_submessage_count()) && (error_location == ADJUDICATOR_NO_ERROR);
         submessage_counter++) {
        error_location = process_supply_centres_for_power(supply_centres.get_submessage(submessage_counter));

        if (error_location != ADJUDICATOR_NO_ERROR) {
            error_location = error_location + supply_centres.get_submessage_start(submessage_counter);
        }
    }

    return error_location;
}

int MapAndUnits::process_supply_centres_for_power(TokenMessage &supply_centres) {
    int error_location = ADJUDICATOR_NO_ERROR;
    int token_counter;
    int submessage_counter;
    HOME_CENTRE_SET home_centre_set;
    Token token;
    int province_index;
    Token power = TOKEN_PARAMETER_UNO;
    TokenMessage submessage;

    for (submessage_counter = 0;
         (submessage_counter < supply_centres.get_submessage_count()) && (error_location == ADJUDICATOR_NO_ERROR);
         submessage_counter++) {
        submessage = supply_centres.get_submessage(submessage_counter);

        if (submessage.is_single_token()) {
            token = submessage.get_token();

            if (token.get_category() == CATEGORY_POWER) {
                if (token.get_subtoken() < number_of_powers) {
                    home_centre_set.insert(token.get_subtoken());
                    power = token;
                } else {
                    error_location = supply_centres.get_submessage_start(submessage_counter);
                }
            } else if (token.is_province()) {
                province_index = token.get_subtoken();

                if (!game_map[province_index].province_in_use) {
                    game_map[province_index].province_token = token;
                    game_map[province_index].province_in_use = true;
                    game_map[province_index].home_centre_set = home_centre_set;
                    game_map[province_index].is_supply_centre = true;
                    game_map[province_index].owner = power;
                } else {
                    error_location = supply_centres.get_submessage_start(submessage_counter);
                }
            } else if (token != TOKEN_PARAMETER_UNO) {
                error_location = supply_centres.get_submessage_start(submessage_counter);
            }
        } else {
            for (token_counter = 0;
                 (token_counter < submessage.get_message_length()) && (error_location == ADJUDICATOR_NO_ERROR);
                 token_counter++) {
                token = submessage.get_token(token_counter);

                if (token.get_category() == CATEGORY_POWER) {
                    if (token.get_subtoken() < number_of_powers) {
                        home_centre_set.insert(token.get_subtoken());
                        power = token;
                    } else {
                        error_location = token_counter + supply_centres.get_submessage_start(submessage_counter);
                    }
                } else {
                    error_location = supply_centres.get_submessage_start(submessage_counter);
                }
            }
        }
    }

    return error_location;
}

int MapAndUnits::process_non_supply_centres(TokenMessage &non_supply_centres) {
    int error_location = ADJUDICATOR_NO_ERROR;
    int token_counter;
    Token token;
    int province_index;

    for (token_counter = 0;
         (token_counter < non_supply_centres.get_message_length()) && (error_location == ADJUDICATOR_NO_ERROR);
         token_counter++) {
        token = non_supply_centres.get_token(token_counter);

        if (token.is_province()) {
            province_index = token.get_subtoken();

            if (!game_map[province_index].province_in_use) {
                game_map[province_index].province_token = token;
                game_map[province_index].province_in_use = true;
                game_map[province_index].owner = TOKEN_PARAMETER_UNO;
            } else {
                error_location = token_counter;
            }
        } else if (token != TOKEN_PARAMETER_UNO) {
            error_location = token_counter;
        }
    }

    return error_location;
}

int MapAndUnits::process_adjacencies(TokenMessage &adjacencies) {
    int error_location = ADJUDICATOR_NO_ERROR;
    TokenMessage province_adjacency;
    int province_counter;

    for (province_counter = 0;
         (province_counter < adjacencies.get_submessage_count()) && (error_location == ADJUDICATOR_NO_ERROR);
         province_counter++) {
        province_adjacency = adjacencies.get_submessage(province_counter);

        error_location = process_province_adjacency(province_adjacency);

        if (error_location != ADJUDICATOR_NO_ERROR) {
            error_location = error_location + adjacencies.get_submessage_start(province_counter);
        }
    }

    return error_location;
}

int MapAndUnits::process_province_adjacency(TokenMessage &province_adjacency) {
    int error_location = ADJUDICATOR_NO_ERROR;
    Token province_token;
    PROVINCE_DETAILS *province_details;
    int adjacency_counter;
    TokenMessage adjacency_list;

    province_token = province_adjacency.get_token(0);

    province_details = &(game_map[province_token.get_subtoken()]);

    if (!province_details->province_in_use
        || !province_details->coast_info.empty()) {
        error_location = 0;
    } else {
        for (adjacency_counter = 1;
             (adjacency_counter < province_adjacency.get_submessage_count() && error_location == ADJUDICATOR_NO_ERROR);
             adjacency_counter++) {
            adjacency_list = province_adjacency.get_submessage(adjacency_counter);

            error_location = process_adjacency_list(province_details, adjacency_list);

            if (error_location != ADJUDICATOR_NO_ERROR) {
                error_location = error_location + province_adjacency.get_submessage_start(adjacency_counter);
            }
        }
    }

    return error_location;
}

int MapAndUnits::process_adjacency_list(PROVINCE_DETAILS *province_details, TokenMessage &adjacency_list) {
    int error_location = ADJUDICATOR_NO_ERROR;
    TokenMessage adjacency_token;
    COAST_DETAILS *coast_details;
    Token coast_token;
    Token adjacent_coast_token;
    int adjacency_counter;
    COAST_ID adjacent_coast;
    Token province_token;

    adjacency_token = adjacency_list.get_submessage(0);

    if (adjacency_token.is_single_token()) {
        coast_token = adjacency_token.get_token();
        adjacent_coast_token = coast_token;

        if (coast_token == TOKEN_UNIT_AMY) {
            province_details->is_land = true;
        }
    } else {
        coast_token = adjacency_token.get_token(1);
        adjacent_coast_token = TOKEN_UNIT_FLT;
    }

    coast_details = &(province_details->coast_info[coast_token]);

    if (!coast_details->adjacent_coasts.empty()) {
        error_location = 0;
    } else {
        for (adjacency_counter = 1;
             (adjacency_counter < adjacency_list.get_submessage_count()) && (error_location == ADJUDICATOR_NO_ERROR);
             adjacency_counter++) {
            adjacency_token = adjacency_list.get_submessage(adjacency_counter);

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
    }

    return error_location;
}

void MapAndUnits::set_power_played(Token power) {
    int province_counter;                // Counter to loop through the provinces
    int power_index;                    // Power index for this power

    // Store the power played
    power_played = power;

    // Build the list of home centres
    home_centres.clear();

    if (power.get_category() == CATEGORY_POWER) {
        power_index = power.get_subtoken();

        for (province_counter = 0; province_counter < number_of_provinces; province_counter++) {
            if (game_map[province_counter].home_centre_set.find(power_index)
                != game_map[province_counter].home_centre_set.end()) {
                home_centres.insert(province_counter);
            }
        }
    }

    // The game has started
    game_started = true;
}

int MapAndUnits::set_ownership(TokenMessage &sco_message) {
    int error_location = ADJUDICATOR_NO_ERROR;
    TokenMessage sco_command;
    int submessage_counter;
    TokenMessage sco_for_power;

    // Check we have a map
    if (number_of_provinces != NO_MAP) {
        sco_command = sco_message.get_submessage(0);

        if (!sco_command.is_single_token()
            || (sco_command.get_token() != TOKEN_COMMAND_SCO)) {
            error_location = 0;
        } else {
            our_centres.clear();

            for (submessage_counter = 1;
                 (submessage_counter < sco_message.get_submessage_count()) && (error_location == ADJUDICATOR_NO_ERROR);
                 submessage_counter++) {
                sco_for_power = sco_message.get_submessage(submessage_counter);

                error_location = process_sco_for_power(sco_for_power);

                if (error_location != ADJUDICATOR_NO_ERROR) {
                    error_location = error_location + sco_message.get_submessage_start(submessage_counter);
                }
            }
        }
    }

    return error_location;
}

int MapAndUnits::process_sco_for_power(TokenMessage &sco_for_power) {
    int error_location = ADJUDICATOR_NO_ERROR;
    Token power;
    Token province;
    int province_counter;

    power = sco_for_power.get_token(0);

    for (province_counter = 1;
         (province_counter < sco_for_power.get_message_length()) && (error_location == ADJUDICATOR_NO_ERROR);
         province_counter++) {
        province = sco_for_power.get_token(province_counter);

        if (province.get_subtoken() >= number_of_provinces) {
            error_location = province_counter;
        } else {
            game_map[province.get_subtoken()].owner = power;

            if (power == power_played) {
                our_centres.insert(province.get_subtoken());
            }
        }
    }

    return error_location;
}

int MapAndUnits::set_units(TokenMessage &now_message) {
    int error_location = ADJUDICATOR_NO_ERROR;
    TokenMessage now_command;
    TokenMessage turn_message;
    TokenMessage unit;
    int unit_counter;
    PROVINCE_SET::iterator home_centre_iterator;

    // Check we have a map
    if (number_of_provinces != NO_MAP) {
        now_command = now_message.get_submessage(0);

        if (!now_command.is_single_token()
            || (now_command.get_token() != TOKEN_COMMAND_NOW)) {
            error_location = 0;
        } else {
            turn_message = now_message.get_submessage(1);

            current_season = turn_message.get_token(0);
            current_year = turn_message.get_token(1).get_number();

            units.clear();
            dislodged_units.clear();
            our_units.clear();
            our_dislodged_units.clear();
            open_home_centres.clear();
            our_winter_orders.builds_or_disbands.clear();
            our_winter_orders.number_of_waives = 0;

            // Store the unit positions
            for (unit_counter = 2;
                 (unit_counter < now_message.get_submessage_count()) && (error_location == ADJUDICATOR_NO_ERROR);
                 unit_counter++) {
                unit = now_message.get_submessage(unit_counter);

                error_location = process_now_unit(unit);

                if (error_location != ADJUDICATOR_NO_ERROR) {
                    error_location = error_location + now_message.get_submessage_start(unit_counter);
                }
            }

            if (power_played != Token(0)) {
                // Build the list of open home centres
                open_home_centres.clear();

                for (home_centre_iterator = home_centres.begin();
                     home_centre_iterator != home_centres.end();
                     home_centre_iterator++) {
                    if ((game_map[*home_centre_iterator].owner == power_played)
                        && (units.find(*home_centre_iterator) == units.end())) {
                        open_home_centres.insert(*home_centre_iterator);
                    }
                }

                // Set the number of builds or disbands
                number_of_disbands = our_units.size() - our_centres.size();
            }
        }
    }

    return error_location;
}

int MapAndUnits::process_now_unit(TokenMessage unit_message) {
    int error_location = ADJUDICATOR_NO_ERROR;
    POWER_INDEX nationality;
    Token unit_type;
    TokenMessage location;
    Token province;
    Token coast;
    UNIT_AND_ORDER unit;
    TokenMessage retreat_option_list;
    int retreat_location_counter;

    nationality = unit_message.get_token(0).get_subtoken();

    if (nationality >= number_of_powers) {
        error_location = 0;
    } else {
        unit_type = unit_message.get_token(1);

        location = unit_message.get_submessage(2);

        if (location.is_single_token()) {
            province = location.get_token();
            coast = unit_type;
        } else {
            if (unit_type != TOKEN_UNIT_FLT) {
                error_location = 2;
            } else {
                province = location.get_token(0);
                coast = location.get_token(1);
            }
        }
    }

    if (error_location == ADJUDICATOR_NO_ERROR) {
        if (province.get_subtoken() >= number_of_provinces) {
            error_location = 2;
        } else if (game_map[province.get_subtoken()].coast_info.find(coast) ==
                   game_map[province.get_subtoken()].coast_info.end()) {
            error_location = 2;
        } else {
            unit.coast_id.province_index = province.get_subtoken();
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

            if (unit_message.get_submessage_count() == 5) {
                // Unit is dislodged
                if (unit_message.get_submessage(3).get_token() != TOKEN_PARAMETER_MRT) {
                    error_location = unit_message.get_submessage_start(3);
                } else {
                    retreat_option_list = unit_message.get_submessage(4);

                    unit.retreat_options.clear();

                    for (retreat_location_counter = 0;
                         retreat_location_counter < retreat_option_list.get_submessage_count();
                         retreat_location_counter++) {
                        unit.retreat_options.insert(
                                get_coast_id(retreat_option_list.get_submessage(retreat_location_counter), unit_type));
                    }
                }

                dislodged_units[province.get_subtoken()] = unit;

                if (nationality == power_played.get_subtoken()) {
                    our_dislodged_units.insert(province.get_subtoken());
                }
            } else {
                // Unit is not dislodged
                units[province.get_subtoken()] = unit;

                if (nationality == power_played.get_subtoken()) {
                    our_units.insert(province.get_subtoken());
                }
            }
        }
    }

    return error_location;
}

int MapAndUnits::store_result(TokenMessage &ord_message) {
    int error_location = ADJUDICATOR_NO_ERROR;
    // The location of any error in the message
    TokenMessage turn;                // The turn from the message
    TokenMessage order;                // The order from the message
    TokenMessage result;            // The result from the message
    Token season;                    // The season from the message
    TokenMessage unit;                // The unit in the orders
    Token order_type;                // The token indicating the order type
    POWER_INDEX power;                // The power which owns the order
    WINTER_ORDERS::iterator adjustment_orders_iterator;
    // Iterator to the winter orders for this power
    WINTER_ORDERS_FOR_POWER new_adjustment_orders;
    // A set of adjustment orders for a power
    UNIT_AND_ORDER new_unit;        // A new unit to add to the results

    if (number_of_provinces == NO_MAP) {
        // No map set up yet, so can't process
    } else if (ord_message.get_submessage_count() != 4) {
        error_location = 0;
    } else if (!ord_message.get_submessage(0).is_single_token()
               || (ord_message.get_submessage(0).get_token() != TOKEN_COMMAND_ORD)) {
        error_location = 0;
    } else {
        turn = ord_message.get_submessage(1);
        order = ord_message.get_submessage(2);
        result = ord_message.get_submessage(3);

        season = turn.get_token(0);

        // If it is the first spring or fall movement result, then clear all the previous results
        if (((season == TOKEN_SEASON_SPR)
             || (season == TOKEN_SEASON_FAL))
            && (season != last_movement_result_season)) {
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
            adjustment_orders_iterator = last_adjustment_results.find(power);

            // If they don't exist, then create them
            if (adjustment_orders_iterator == last_adjustment_results.end()) {
                new_adjustment_orders.builds_or_disbands.clear();
                new_adjustment_orders.number_of_waives = 0;

                adjustment_orders_iterator = last_adjustment_results.insert(
                        WINTER_ORDERS::value_type(power, new_adjustment_orders)).first;
            }

            // Add it to the adjustment results
            if (order_type == TOKEN_ORDER_WVE) {
                adjustment_orders_iterator->second.number_of_waives++;
            } else if (order_type == TOKEN_ORDER_BLD) {
                adjustment_orders_iterator->second.builds_or_disbands.insert(
                        BUILDS_OR_DISBANDS::value_type(get_coast_from_unit(unit), TOKEN_RESULT_SUC));
                adjustment_orders_iterator->second.is_building = true;
            } else {
                adjustment_orders_iterator->second.builds_or_disbands.insert(
                        BUILDS_OR_DISBANDS::value_type(get_coast_from_unit(unit), TOKEN_RESULT_SUC));
                adjustment_orders_iterator->second.is_building = false;
            }
        } else {
            // Set up the new unit
            new_unit.coast_id = get_coast_from_unit(unit);
            new_unit.nationality = power;
            new_unit.unit_type = unit.get_token(1);

            // Decode the order
            decode_order(new_unit, order);

            // Decode the result
            decode_result(new_unit, result);

            if ((season == TOKEN_SEASON_SPR)
                || (season == TOKEN_SEASON_FAL)) {
                // Add it to the movement results
                last_movement_results.insert(UNITS::value_type(new_unit.coast_id.province_index, new_unit));
            } else {
                // Add it to the retreat results
                last_retreat_results.insert(UNITS::value_type(new_unit.coast_id.province_index, new_unit));
            }
        }
    }

    return error_location;
}

bool MapAndUnits::set_hold_order(PROVINCE_INDEX unit) {
    UNITS::iterator unit_to_order;            // The unit to order
    bool unit_ordered = true;                // Whether the unit was ordered successfully

    unit_to_order = units.find(unit);

    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = HOLD_ORDER;
    }

    return unit_ordered;
}

bool MapAndUnits::set_move_order(PROVINCE_INDEX unit, COAST_ID destination) {
    UNITS::iterator unit_to_order;            // The unit to order
    bool unit_ordered = true;                // Whether the unit was ordered successfully

    unit_to_order = units.find(unit);

    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = MOVE_ORDER;
        unit_to_order->second.move_dest = destination;
    }

    return unit_ordered;
}

bool MapAndUnits::set_support_to_hold_order(PROVINCE_INDEX unit, PROVINCE_INDEX supported_unit) {
    UNITS::iterator unit_to_order;            // The unit to order
    bool unit_ordered = true;                // Whether the unit was ordered successfully

    unit_to_order = units.find(unit);

    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = SUPPORT_TO_HOLD_ORDER;
        unit_to_order->second.other_dest_province = supported_unit;
    }

    return unit_ordered;
}

bool
MapAndUnits::set_support_to_move_order(PROVINCE_INDEX unit, PROVINCE_INDEX supported_unit, PROVINCE_INDEX destination) {
    UNITS::iterator unit_to_order;            // The unit to order
    bool unit_ordered = true;                // Whether the unit was ordered successfully

    unit_to_order = units.find(unit);

    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = SUPPORT_TO_MOVE_ORDER;
        unit_to_order->second.other_source_province = supported_unit;
        unit_to_order->second.other_dest_province = destination;
    }

    return unit_ordered;
}

bool MapAndUnits::set_convoy_order(PROVINCE_INDEX unit, PROVINCE_INDEX convoyed_army, PROVINCE_INDEX destination) {
    UNITS::iterator unit_to_order;            // The unit to order
    bool unit_ordered = true;                // Whether the unit was ordered successfully

    unit_to_order = units.find(unit);

    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = CONVOY_ORDER;
        unit_to_order->second.other_source_province = convoyed_army;
        unit_to_order->second.other_dest_province = destination;
    }

    return unit_ordered;
}

bool MapAndUnits::set_move_by_convoy_order(PROVINCE_INDEX unit, PROVINCE_INDEX destination, int number_of_steps,
                                           PROVINCE_INDEX step_list[]) {
    UNITS::iterator unit_to_order;            // The unit to order
    bool unit_ordered = true;                // Whether the unit was ordered successfully
    int step_counter;                        // Counter to loop through the convoy steps

    unit_to_order = units.find(unit);

    if (unit_to_order == units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = MOVE_BY_CONVOY_ORDER;
        unit_to_order->second.move_dest.province_index = destination;
        unit_to_order->second.move_dest.coast_token = TOKEN_UNIT_AMY;
        unit_to_order->second.convoy_step_list.clear();

        for (step_counter = 0; step_counter < number_of_steps; step_counter++) {
            unit_to_order->second.convoy_step_list.push_back(step_list[step_counter]);
        }
    }

    return unit_ordered;
}

bool MapAndUnits::set_move_by_single_step_convoy_order(PROVINCE_INDEX unit, PROVINCE_INDEX destination,
                                                       PROVINCE_INDEX step) {
    return set_move_by_convoy_order(unit, destination, 1, &step);
}

bool MapAndUnits::set_disband_order(PROVINCE_INDEX unit) {
    UNITS::iterator unit_to_order;            // The unit to order
    bool unit_ordered = true;                // Whether the unit was ordered successfully

    unit_to_order = dislodged_units.find(unit);

    if (unit_to_order == dislodged_units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = DISBAND_ORDER;
    }

    return unit_ordered;
}

bool MapAndUnits::set_retreat_order(PROVINCE_INDEX unit, COAST_ID destination) {
    UNITS::iterator unit_to_order;            // The unit to order
    bool unit_ordered = true;                // Whether the unit was ordered successfully

    unit_to_order = dislodged_units.find(unit);

    if (unit_to_order == dislodged_units.end()) {
        unit_ordered = false;
    } else {
        unit_to_order->second.order_type = RETREAT_ORDER;
        unit_to_order->second.move_dest = destination;
    }

    return unit_ordered;
}

void MapAndUnits::set_build_order(COAST_ID location) {
    COAST_ID min_coast;                    // Minimum coast for this province
    BUILDS_OR_DISBANDS::iterator matching_unit;
    // Unit already in the build list which is in the same location

    min_coast.province_index = location.province_index;
    min_coast.coast_token = Token(0, 0);

    matching_unit = our_winter_orders.builds_or_disbands.lower_bound(min_coast);

    if ((matching_unit != our_winter_orders.builds_or_disbands.end())
        && (matching_unit->first.province_index == location.province_index)) {
        our_winter_orders.builds_or_disbands.erase(matching_unit);
    }

    our_winter_orders.builds_or_disbands.insert(
            BUILDS_OR_DISBANDS::value_type(location, Token(0)));
    our_winter_orders.is_building = true;
}

bool MapAndUnits::set_remove_order(PROVINCE_INDEX unit) {
    bool unit_ordered = true;                // Whether the unit was ordered successfully
    UNITS::iterator unit_iterator;            // Iterator to the unit to remove

    unit_iterator = units.find(unit);

    if (unit_iterator == units.end()) {
        unit_ordered = false;
    } else {
        our_winter_orders.builds_or_disbands.insert(
                BUILDS_OR_DISBANDS::value_type(unit_iterator->second.coast_id, Token(0)));
        our_winter_orders.is_building = false;
    }

    return unit_ordered;
}

void MapAndUnits::set_waive_order() {
    our_winter_orders.number_of_waives++;
}

void MapAndUnits::set_multiple_waive_orders(int waives) {
    our_winter_orders.number_of_waives = our_winter_orders.number_of_waives + waives;
}

void MapAndUnits::set_total_number_of_waive_orders(int waives) {
    our_winter_orders.number_of_waives = waives;
}

bool MapAndUnits::cancel_build_order(PROVINCE_INDEX location) {
    COAST_ID min_coast;                    // Minimum coast for this province
    BUILDS_OR_DISBANDS::iterator matching_unit;
    // Unit already in the build list which is in the same location
    bool order_found;                    // Whether a matching order was found

    min_coast.province_index = location;
    min_coast.coast_token = Token(0, 0);

    matching_unit = our_winter_orders.builds_or_disbands.lower_bound(min_coast);

    if ((matching_unit != our_winter_orders.builds_or_disbands.end())
        && (matching_unit->first.province_index == location)) {
        our_winter_orders.builds_or_disbands.erase(matching_unit);

        order_found = true;
    } else {
        order_found = false;
    }

    return order_found;
}

bool MapAndUnits::cancel_remove_order(PROVINCE_INDEX location) {
    // This is exactly the same as removing a build order!
    return cancel_build_order(location);
}

bool MapAndUnits::any_orders_entered() {
    UNITS::iterator unit_iterator;            // The unit we are working on
    bool order_entered = false;                // Whether any orders have been entered

    if ((current_season == TOKEN_SEASON_SPR) || (current_season == TOKEN_SEASON_FAL)) {
        for (unit_iterator = units.begin(); unit_iterator != units.end(); unit_iterator++) {
            if (unit_iterator->second.order_type != NO_ORDER) {
                order_entered = true;
            }
        }
    } else if ((current_season == TOKEN_SEASON_SUM) || (current_season == TOKEN_SEASON_AUT)) {
        for (unit_iterator = dislodged_units.begin(); unit_iterator != dislodged_units.end(); unit_iterator++) {
            if (unit_iterator->second.order_type != NO_ORDER) {
                order_entered = true;
            }
        }
    } else {
        if (!our_winter_orders.builds_or_disbands.empty()
            || (our_winter_orders.number_of_waives != 0)) {
            order_entered = true;
        }
    }

    return order_entered;
}

TokenMessage MapAndUnits::build_sub_command() {
    TokenMessage sub_message;                // The sub message
    UNITS::iterator unit_iterator;            // The unit we are working on
    TokenMessage unit_order;                // The order for one unit
    BUILDS_OR_DISBANDS::iterator build_iterator;
    // Iterator to loop through the builds
    int waive_counter;                        // Counter to loop through the waives

    sub_message = TOKEN_COMMAND_SUB;

    if ((current_season == TOKEN_SEASON_SPR) || (current_season == TOKEN_SEASON_FAL)) {
        for (unit_iterator = units.begin(); unit_iterator != units.end(); unit_iterator++) {
            if ((unit_iterator->second.nationality == power_played.get_subtoken())
                && (unit_iterator->second.order_type != NO_ORDER)) {
                unit_order = describe_movement_order(&(unit_iterator->second));

                sub_message = sub_message & unit_order;
            }
        }
    } else if ((current_season == TOKEN_SEASON_SUM) || (current_season == TOKEN_SEASON_AUT)) {
        for (unit_iterator = dislodged_units.begin(); unit_iterator != dislodged_units.end(); unit_iterator++) {
            if ((unit_iterator->second.nationality == power_played.get_subtoken())
                && (unit_iterator->second.order_type != NO_ORDER)) {
                unit_order = describe_retreat_order(&(unit_iterator->second));

                sub_message = sub_message & unit_order;
            }
        }
    } else {
        for (build_iterator = our_winter_orders.builds_or_disbands.begin();
             build_iterator != our_winter_orders.builds_or_disbands.end();
             build_iterator++) {
            unit_order = power_played;

            if (build_iterator->first.coast_token == TOKEN_UNIT_AMY) {
                unit_order = unit_order + TOKEN_UNIT_AMY;
            } else {
                unit_order = unit_order + TOKEN_UNIT_FLT;
            }

            unit_order = unit_order + describe_coast(build_iterator->first);

            unit_order.enclose_this();

            if (our_winter_orders.is_building) {
                unit_order = unit_order + TOKEN_ORDER_BLD;
            } else {
                unit_order = unit_order + TOKEN_ORDER_REM;
            }

            sub_message = sub_message & unit_order;
        }

        for (waive_counter = 0; waive_counter < our_winter_orders.number_of_waives; waive_counter++) {
            unit_order = power_played + TOKEN_ORDER_WVE;

            sub_message = sub_message & unit_order;
        }
    }

    return sub_message;
}

void MapAndUnits::clear_all_orders() {
    UNITS::iterator unit_iterator;

    for (unit_iterator = units.begin(); unit_iterator != units.end(); unit_iterator++) {
        unit_iterator->second.order_type = NO_ORDER;
    }

    for (unit_iterator = dislodged_units.begin(); unit_iterator != dislodged_units.end(); unit_iterator++) {
        unit_iterator->second.order_type = NO_ORDER;
    }

    our_winter_orders.builds_or_disbands.clear();
    our_winter_orders.number_of_waives = 0;
}

MapAndUnits::COAST_ID MapAndUnits::get_coast_id(TokenMessage &coast, Token unit_type) {
    COAST_ID coast_id;
    Token province_token;

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
    TokenMessage order;
    UNIT_LIST::iterator convoy_step_iterator;
    TokenMessage convoy_via;

    switch (unit->order_type) {
        case NO_ORDER:
        case HOLD_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_HLD;

            break;
        }

        case MOVE_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_MTO + describe_coast(unit->move_dest);

            break;
        }

        case SUPPORT_TO_HOLD_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_SUP
                    + describe_unit(&(units[unit->other_dest_province]));

            break;
        }

        case SUPPORT_TO_MOVE_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_SUP
                    + describe_unit(&(units[unit->other_source_province])) + TOKEN_ORDER_MTO
                    + game_map[unit->other_dest_province].province_token;

            break;
        }

        case CONVOY_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_CVY
                    + describe_unit(&(units[unit->other_source_province])) + TOKEN_ORDER_CTO
                    + game_map[unit->other_dest_province].province_token;

            break;
        }

        case MOVE_BY_CONVOY_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_CTO + describe_coast(unit->move_dest);

            for (convoy_step_iterator = unit->convoy_step_list.begin();
                 convoy_step_iterator != unit->convoy_step_list.end();
                 convoy_step_iterator++) {
                convoy_via = convoy_via + game_map[*convoy_step_iterator].province_token;
            }

            order = order + TOKEN_ORDER_VIA & convoy_via;

            break;
        }
    }

    return order;
}

TokenMessage MapAndUnits::describe_coast(COAST_ID coast) {
    TokenMessage coast_message;

    if (coast.coast_token.get_category() == CATEGORY_COAST) {
        coast_message = game_map[coast.province_index].province_token + coast.coast_token;
        coast_message.enclose_this();
    } else {
        coast_message = game_map[coast.province_index].province_token;
    }

    return coast_message;
}

TokenMessage MapAndUnits::describe_retreat_order(UNIT_AND_ORDER *unit) {
    TokenMessage order;

    switch (unit->order_type) {
        case NO_ORDER:
        case DISBAND_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_DSB;

            break;
        }

        case RETREAT_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_RTO + describe_coast(unit->move_dest);

            break;
        }
    }

    return order;
}

MapAndUnits::COAST_ID MapAndUnits::get_coast_from_unit(TokenMessage &unit) {
    return get_coast_id(unit.get_submessage(2), unit.get_submessage(1).get_token());
}

void MapAndUnits::decode_order(UNIT_AND_ORDER &unit, TokenMessage order) {
    Token order_type;                        // The type of order
    TokenMessage convoy_step_list;            // The list of steps a convoy goes through
    int step_counter;                        // Counter to step through the convoy steps

    order_type = order.get_submessage(1).get_token();

    if (order_type == TOKEN_ORDER_HLD) {
        // Hold
        unit.order_type = HOLD_ORDER;
    } else if (order_type == TOKEN_ORDER_MTO) {
        // Move
        unit.order_type = MOVE_ORDER;

        unit.move_dest = get_coast_id(order.get_submessage(2), unit.unit_type);
    } else if (order_type == TOKEN_ORDER_SUP) {
        if (order.get_submessage_count() == 3) {
            // Support to hold
            unit.order_type = SUPPORT_TO_HOLD_ORDER;

            unit.other_dest_province = get_coast_from_unit(order.get_submessage(2)).province_index;
        } else {
            // Support to move
            unit.order_type = SUPPORT_TO_MOVE_ORDER;

            unit.other_source_province = get_coast_from_unit(order.get_submessage(2)).province_index;
            unit.other_dest_province = order.get_submessage(4).get_token().get_subtoken();
        }
    } else if (order_type == TOKEN_ORDER_CVY) {
        // Convoy order
        unit.order_type = CONVOY_ORDER;

        unit.other_source_province = get_coast_from_unit(order.get_submessage(2)).province_index;
        unit.other_dest_province = order.get_submessage(4).get_token().get_subtoken();
    } else if (order_type == TOKEN_ORDER_CTO) {
        // Move by convoy
        unit.order_type = MOVE_BY_CONVOY_ORDER;

        unit.move_dest = get_coast_id(order.get_submessage(2), unit.unit_type);
        unit.convoy_step_list.clear();

        convoy_step_list = order.get_submessage(4);

        for (step_counter = 0; step_counter < convoy_step_list.get_message_length(); step_counter++) {
            unit.convoy_step_list.push_back(convoy_step_list.get_token(step_counter).get_subtoken());
        }
    } else if (order_type == TOKEN_ORDER_DSB) {
        // Disband
        unit.order_type = DISBAND_ORDER;
    } else if (order_type == TOKEN_ORDER_RTO) {
        // Retreat
        unit.order_type = RETREAT_ORDER;

        unit.move_dest = get_coast_id(order.get_submessage(2), unit.unit_type);
    }
}

void MapAndUnits::decode_result(UNIT_AND_ORDER &unit, TokenMessage result) {
    int result_counter;                    // Counter to loop through the result tokens
    Token result_token;                    // Token representing one part of the result

    unit.no_convoy = false;
    unit.no_army_to_convoy = false;
    unit.convoy_broken = false;
    unit.support_void = false;
    unit.support_cut = false;
    unit.bounce = false;
    unit.dislodged = false;
    unit.unit_moves = false;
    unit.illegal_order = false;

    for (result_counter = 0; result_counter < result.get_message_length(); result_counter++) {
        result_token = result.get_token(result_counter);

        if (result_token.get_category() == CATEGORY_ORDER_NOTE) {
            unit.illegal_order = true;
            unit.illegal_reason = result_token;
        } else if (result_token == TOKEN_RESULT_SUC) {
            if ((unit.order_type == MOVE_ORDER)
                || (unit.order_type == MOVE_BY_CONVOY_ORDER)
                || (unit.order_type == RETREAT_ORDER)) {
                unit.unit_moves = true;
            }
        } else if (result_token == TOKEN_RESULT_BNC) {
            unit.bounce = true;
        } else if (result_token == TOKEN_RESULT_CUT) {
            unit.support_cut = true;
        } else if (result_token == TOKEN_RESULT_DSR) {
            unit.convoy_broken = true;
        } else if (result_token == TOKEN_RESULT_NSO) {
            if ((unit.order_type == SUPPORT_TO_HOLD_ORDER)
                || (unit.order_type == SUPPORT_TO_MOVE_ORDER)) {
                unit.support_void = true;
            } else if (unit.order_type == CONVOY_ORDER) {
                unit.no_army_to_convoy = true;
            } else if (unit.order_type == MOVE_BY_CONVOY_ORDER) {
                unit.no_convoy = true;
            }
        } else if (result_token == TOKEN_RESULT_RET) {
            unit.dislodged = true;
        }
    }
}

string MapAndUnits::describe_movement_result(UNIT_AND_ORDER &unit) {
    string order;

    order = describe_movement_order_string(unit, last_movement_results);

    if (unit.bounce) {
        order = order + " (bounce)";
    } else if (unit.support_cut) {
        order = order + " (cut)";
    } else if ((unit.no_convoy) || (unit.no_army_to_convoy) || (unit.support_void)) {
        order = order + " (void)";
    } else if (unit.convoy_broken) {
        order = order + " (no convoy)";
    } else if (unit.illegal_order) {
        order = order + " (illegal)";
    }

    if (unit.dislodged) {
        order = order + " (dislodged)";
    }

    return order;
}

string MapAndUnits::describe_movement_order_string(UNIT_AND_ORDER &unit, UNITS &unit_set) {
    string order;
    UNIT_LIST::iterator convoy_step_iterator;
    string convoy_via;

    switch (unit.order_type) {
        case HOLD_ORDER: {
            order = describe_unit_string(unit) + " Hold";

            break;
        }

        case MOVE_ORDER: {
            order = describe_unit_string(unit) + " - " + describe_coast_string(unit.move_dest);

            break;
        }

        case SUPPORT_TO_HOLD_ORDER: {
            order = describe_unit_string(unit) + " s "
                    + describe_unit_string(unit_set[unit.other_dest_province]);

            break;
        }

        case SUPPORT_TO_MOVE_ORDER: {
            order = describe_unit_string(unit) + " s "
                    + describe_unit_string(unit_set[unit.other_source_province])
                    + " - " + describe_province_string(unit.other_dest_province);

            break;
        }

        case CONVOY_ORDER: {
            order = describe_unit_string(unit) + " c "
                    + describe_unit_string(unit_set[unit.other_source_province])
                    + " - " + describe_province_string(unit.other_dest_province);

            break;
        }

        case MOVE_BY_CONVOY_ORDER: {
            order = describe_unit_string(unit);

            for (convoy_step_iterator = unit.convoy_step_list.begin();
                 convoy_step_iterator != unit.convoy_step_list.end();
                 convoy_step_iterator++) {
                order = order + " - " + describe_province_string(*convoy_step_iterator);
            }

            order = order + " - " + describe_coast_string(unit.move_dest);

            break;
        }
    }

    return order;
}

string MapAndUnits::describe_retreat_result(UNIT_AND_ORDER &unit) {
    string order;

    order = describe_retreat_order_string(unit);

    if (unit.bounce) {
        order = order + "(bounce)";
    }

    return order;
}

string MapAndUnits::describe_retreat_order_string(UNIT_AND_ORDER &unit) {
    string order;

    switch (unit.order_type) {
        case DISBAND_ORDER: {
            order = describe_unit_string(unit) + " Disband";

            break;
        }

        case RETREAT_ORDER: {
            order = describe_unit_string(unit) + " - " + describe_coast_string(unit.move_dest);

            break;
        }
    }

    return order;
}

// Also works with maps...
template<class SetType>
typename SetType::value_type get_from_set(SetType my_set, int item_index) {
    int item_counter;
    SetType::iterator set_iterator;

    set_iterator = my_set.begin();

    for (item_counter = 0; item_counter < item_index; item_counter++) {
        set_iterator++;
    }

    return (*set_iterator);
}

string MapAndUnits::describe_adjustment_result(WINTER_ORDERS_FOR_POWER &orders, int order_index) {
    string order;                            // The order description
    COAST_ID build_order;                    // The build order to describe

    if (order_index < (int) orders.builds_or_disbands.size()) {
        if (orders.is_building) {
            order = "Build";
        } else {
            order = "Remove";
        }

        build_order = get_from_set<BUILDS_OR_DISBANDS>(orders.builds_or_disbands, order_index).first;

        if (build_order.coast_token == TOKEN_UNIT_AMY) {
            order = " A(";
        } else {
            order = " F(";
        }

        order = order + describe_coast_string(build_order) + ")";
    } else {
        order = "Waive";
    }

    return order;
}

int MapAndUnits::get_number_of_results(WINTER_ORDERS_FOR_POWER &orders) {
    return (orders.builds_or_disbands.size() + orders.number_of_waives);
}

string MapAndUnits::describe_unit_string(UNIT_AND_ORDER &unit) {
    string unit_string;

    if (unit.unit_type == TOKEN_UNIT_AMY) {
        unit_string = "A(";
    } else {
        unit_string = "F(";
    }

    unit_string = unit_string + describe_coast_string(unit.coast_id) + ")";

    return unit_string;
}

string MapAndUnits::describe_coast_string(COAST_ID &coast) {
    string coast_string;

    coast_string = describe_province_string(coast.province_index);

    if (coast.coast_token == TOKEN_COAST_NCS) {
        coast_string = coast_string + "/nc";
    } else if (coast.coast_token == TOKEN_COAST_SCS) {
        coast_string = coast_string + "/sc";
    } else if (coast.coast_token == TOKEN_COAST_ECS) {
        coast_string = coast_string + "/ec";
    } else if (coast.coast_token == TOKEN_COAST_WCS) {
        coast_string = coast_string + "/wc";
    }

    return coast_string;
}

string MapAndUnits::describe_province_string(PROVINCE_INDEX province_index) {
    TokenMessage province_token;            // The token representing this province
    string province_upper;                    // The province abbreviation in uppoer case
    string province_abbrv;                    // The province abbreviation
    int chr_counter;                        // Counter to loop through the chrs in the abbreviation

    province_token = game_map[province_index].province_token;
    province_upper = province_token.get_message_as_text();

    if (game_map[province_index].is_land) {
        province_abbrv = province_upper[0];

        for (chr_counter = 1; chr_counter < (int) province_upper.size(); chr_counter++) {
            if (province_upper[chr_counter] != ' ') {
                province_abbrv = province_abbrv + (char) (tolower(province_upper[chr_counter]));
            }
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

MapAndUnits::COAST_ID MapAndUnits::find_result_unit_initial_location(
        MapAndUnits::PROVINCE_INDEX province_index,
        bool &is_new_build,
        bool &retreated_to_province,
        bool &moved_to_province,
        bool &unit_found) {
    UNITS::iterator unit_iterator;                // Iterator to step through the units
    WINTER_ORDERS::iterator winter_iterator;    // Iterator through the orders for winter
    COAST_ID initial_location;                    // The initial location of the unit
    BUILDS_OR_DISBANDS::iterator build_iterator;// Iterator to step through the builds for a power

    is_new_build = false;
    retreated_to_province = false;
    moved_to_province = false;
    unit_found = false;

    for (winter_iterator = last_adjustment_results.begin();
         winter_iterator != last_adjustment_results.end();
         winter_iterator++) {
        if (winter_iterator->second.is_building) {
            for (build_iterator = winter_iterator->second.builds_or_disbands.begin();
                 build_iterator != winter_iterator->second.builds_or_disbands.end();
                 build_iterator++) {
                if (build_iterator->first.province_index == province_index) {
                    initial_location = build_iterator->first;
                    is_new_build = true;
                    unit_found = true;
                }
            }
        }
    }

    if (!unit_found) {
        for (unit_iterator = last_retreat_results.begin();
             unit_iterator != last_retreat_results.end();
             unit_iterator++) {
            if ((unit_iterator->second.move_dest.province_index == province_index)
                && (unit_iterator->second.unit_moves)) {
                initial_location = unit_iterator->second.coast_id;
                retreated_to_province = true;
                unit_found = true;
            }
        }
    }

    if (!unit_found) {
        for (unit_iterator = last_movement_results.begin();
             unit_iterator != last_movement_results.end();
             unit_iterator++) {
            if ((unit_iterator->second.move_dest.province_index == province_index)
                && (unit_iterator->second.unit_moves)) {
                initial_location = unit_iterator->second.coast_id;
                moved_to_province = true;
                unit_found = true;
            } else if ((unit_iterator->second.coast_id.province_index == province_index)
                       && !unit_iterator->second.unit_moves
                       && !unit_iterator->second.dislodged) {
                initial_location = unit_iterator->second.coast_id;
                unit_found = true;
            }
        }
    }

    return initial_location;
}

bool MapAndUnits::get_variant_setting(const Token &variant_option, Token *parameter) {
    TokenMessage variant_submessage;
    int submessage_counter;
    bool variant_found = false;

    // For each variant option
    for (submessage_counter = 0; submessage_counter < variant.get_submessage_count(); submessage_counter++) {
        variant_submessage = variant.get_submessage(submessage_counter);

        // If it is the right one
        if (variant_submessage.get_token() == variant_option) {
            variant_found = true;

            // Get the parameter
            if ((variant_submessage.get_message_length() > 1)
                && (parameter != NULL)) {
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
    COAST_SET *adjacent_coasts;
    UNITS::iterator unit_iterator;

    unit_iterator = units.find(unit_location);

    if (unit_iterator == units.end()) {
        adjacent_coasts = NULL;
    } else {
        adjacent_coasts = get_adjacent_coasts(unit_iterator->second.coast_id);
    }

    return adjacent_coasts;
}

MapAndUnits::COAST_SET *MapAndUnits::get_dislodged_unit_adjacent_coasts(
        PROVINCE_INDEX &dislodged_unit_location) {
    COAST_SET *adjacent_coasts;
    UNITS::iterator unit_iterator;

    unit_iterator = dislodged_units.find(dislodged_unit_location);

    if (unit_iterator == dislodged_units.end()) {
        adjacent_coasts = NULL;
    } else {
        adjacent_coasts = get_adjacent_coasts(unit_iterator->second.coast_id);
    }

    return adjacent_coasts;
}

void MapAndUnits::set_order_checking(bool check_on_submit, bool check_on_adjudicate) {
    check_orders_on_submission = check_on_submit;
    check_orders_on_adjudication = check_on_adjudicate;
}

int MapAndUnits::process_orders(TokenMessage &sub_message, POWER_INDEX power_index, Token *order_result) {
    int error_location = ADJUDICATOR_NO_ERROR;
    TokenMessage sub_command;
    int submessage_counter;
    TokenMessage order;

    sub_command = sub_message.get_submessage(0);

    if (!sub_command.is_single_token()
        || (sub_command.get_token() != TOKEN_COMMAND_SUB)) {
        error_location = 0;
    } else {
        for (submessage_counter = 1;
             (submessage_counter < sub_message.get_submessage_count()) && (error_location == ADJUDICATOR_NO_ERROR);
             submessage_counter++) {
            order = sub_message.get_submessage(submessage_counter);

            order_result[submessage_counter - 1] = process_order(order, power_index);
        }
    }

    return error_location;
}

Token MapAndUnits::process_order(TokenMessage &order, POWER_INDEX power_index) {
    Token order_result = TOKEN_ORDER_NOTE_MBV;
    TokenMessage order_token_message;
    Token order_token;
    UNIT_AND_ORDER *unit_record;
    COAST_ID destination;
    UNIT_AND_ORDER *supported_unit;
    Token support_destination;
    UNIT_AND_ORDER *convoyed_unit;
    Token convoy_destination;
    TokenMessage convoy_via_list;
    UNITS::iterator convoying_unit_iterator;
    UNIT_AND_ORDER *convoying_unit;
    int previous_province;
    int step_counter;
    int convoying_province;
    WINTER_ORDERS_FOR_POWER *winter_record;
    TokenMessage winter_order_unit;
    COAST_ID build_location;
    COAST_ID build_province;
    BUILDS_OR_DISBANDS::iterator match_iterator;

    order_token_message = order.get_submessage(1);

    order_token = order_token_message.get_token();

    if ((order_token.get_token() & ORDER_TURN_MASK) == ORDER_MOVE_TURN_CHECK) {
        if ((current_season != TOKEN_SEASON_SPR)
            && (current_season != TOKEN_SEASON_FAL)) {
            order_result = TOKEN_ORDER_NOTE_NRS;
        } else {
            unit_record = find_unit(order.get_submessage(0), units);

            if (unit_record == NULL) {
                order_result = TOKEN_ORDER_NOTE_NSU;
            } else if (unit_record->nationality != power_index) {
                order_result = TOKEN_ORDER_NOTE_NYU;
            }
        }
    } else if ((order_token.get_token() & ORDER_TURN_MASK) == ORDER_RETREAT_TURN_CHECK) {
        if ((current_season != TOKEN_SEASON_SUM)
            && (current_season != TOKEN_SEASON_AUT)) {
            order_result = TOKEN_ORDER_NOTE_NRS;
        } else {
            unit_record = find_unit(order.get_submessage(0), dislodged_units);

            if (unit_record == NULL) {
                order_result = TOKEN_ORDER_NOTE_NRN;
            } else if (unit_record->nationality != power_index) {
                order_result = TOKEN_ORDER_NOTE_NYU;
            }
        }
    } else if ((order_token.get_token() & ORDER_TURN_MASK) == ORDER_BUILD_TURN_CHECK) {
        if (current_season != TOKEN_SEASON_WIN) {
            order_result = TOKEN_ORDER_NOTE_NRS;
        } else {
            winter_record = &(winter_orders[power_index]);
        }
    }

    if (order_result == TOKEN_ORDER_NOTE_MBV) {
        if (order_token == TOKEN_ORDER_HLD) {
            unit_record->order_type = HOLD_ORDER;
        } else if (order_token == TOKEN_ORDER_MTO) {
            destination = get_coast_id(order.get_submessage(2), unit_record->unit_type);

            if ((check_orders_on_submission)
                && !can_move_to(unit_record, destination)) {
                order_result = TOKEN_ORDER_NOTE_FAR;
            } else {
                unit_record->order_type = MOVE_ORDER;
                unit_record->move_dest = destination;
            }
        } else if (order_token == TOKEN_ORDER_SUP) {
            if (order.get_submessage_count() == 3) {
                // Support to hold
                supported_unit = find_unit(order.get_submessage(2), units);

                if (supported_unit == NULL) {
                    order_result = TOKEN_ORDER_NOTE_NSU;
                } else if ((check_orders_on_submission)
                           && !can_move_to_province(unit_record, supported_unit->coast_id.province_index)) {
                    order_result = TOKEN_ORDER_NOTE_FAR;
                }
                    // Check it isn't trying to support itself
                else if ((check_orders_on_submission)
                         && (supported_unit->coast_id.province_index == unit_record->coast_id.province_index)) {
                    order_result = TOKEN_ORDER_NOTE_FAR;
                } else {
                    unit_record->order_type = SUPPORT_TO_HOLD_ORDER;
                    unit_record->other_source_province = supported_unit->coast_id.province_index;
                }
            } else {
                // Support to move
                supported_unit = find_unit(order.get_submessage(2), units);
                support_destination = order.get_submessage(4).get_token();

                if (supported_unit == NULL) {
                    order_result = TOKEN_ORDER_NOTE_NSU;
                } else if ((check_orders_on_submission)
                           && !has_route_to_province(supported_unit, support_destination.get_subtoken(),
                                                     unit_record->coast_id.province_index)) {
                    order_result = TOKEN_ORDER_NOTE_FAR;
                } else if ((check_orders_on_submission)
                           && !can_move_to_province(unit_record, support_destination.get_subtoken())) {
                    order_result = TOKEN_ORDER_NOTE_FAR;
                }
                    // Check it isn't trying to support itself
                else if ((check_orders_on_submission)
                         && (supported_unit->coast_id.province_index == unit_record->coast_id.province_index)) {
                    order_result = TOKEN_ORDER_NOTE_FAR;
                } else {
                    unit_record->order_type = SUPPORT_TO_MOVE_ORDER;
                    unit_record->other_source_province = supported_unit->coast_id.province_index;
                    unit_record->other_dest_province = support_destination.get_subtoken();
                }
            }
        } else if (order_token == TOKEN_ORDER_CVY) {
            convoyed_unit = find_unit(order.get_submessage(2), units);
            convoy_destination = order.get_submessage(4).get_token();

            if (convoyed_unit == NULL) {
                order_result = TOKEN_ORDER_NOTE_NSU;
            } else if ((check_orders_on_submission)
                       && (unit_record->unit_type != TOKEN_UNIT_FLT)) {
                order_result = TOKEN_ORDER_NOTE_NSF;
            } else if ((check_orders_on_submission)
                       && (game_map[unit_record->coast_id.province_index].is_land)) {
                order_result = TOKEN_ORDER_NOTE_NAS;
            } else if ((check_orders_on_submission)
                       && (convoyed_unit->unit_type != TOKEN_UNIT_AMY)) {
                order_result = TOKEN_ORDER_NOTE_NSA;
            } else if ((check_orders_on_submission)
                       && !has_route_to_province(convoyed_unit, convoy_destination.get_subtoken(), -1)) {
                order_result = TOKEN_ORDER_NOTE_FAR;
            } else {
                unit_record->order_type = CONVOY_ORDER;
                unit_record->other_source_province = convoyed_unit->coast_id.province_index;
                unit_record->other_dest_province = convoy_destination.get_subtoken();
            }
        } else if (order_token == TOKEN_ORDER_CTO) {
            if ((check_orders_on_submission)
                && (unit_record->unit_type != TOKEN_UNIT_AMY)) {
                order_result = TOKEN_ORDER_NOTE_NSA;
            } else {
                convoy_destination = order.get_submessage(2).get_token();
                convoy_via_list = order.get_submessage(4);

                previous_province = unit_record->coast_id.province_index;

                if (check_orders_on_submission) {
                    for (step_counter = 0;
                         (step_counter < convoy_via_list.get_message_length()) &&
                         (order_result == TOKEN_ORDER_NOTE_MBV);
                         step_counter++) {
                        convoying_unit_iterator = units.find(
                                convoy_via_list.get_submessage(step_counter).get_token().get_subtoken());

                        if (convoying_unit_iterator == units.end()) {
                            convoying_unit = NULL;
                        } else {
                            convoying_unit = &(convoying_unit_iterator->second);
                        }

                        if (convoying_unit == NULL) {
                            order_result = TOKEN_ORDER_NOTE_NSF;
                        } else if (game_map[convoying_unit->coast_id.province_index].is_land) {
                            order_result = TOKEN_ORDER_NOTE_NAS;
                        } else if (!can_move_to_province(convoying_unit, previous_province)) {
                            order_result = TOKEN_ORDER_NOTE_FAR;
                        } else {
                            previous_province = convoying_unit->coast_id.province_index;
                        }
                    }

                    if (convoy_destination.get_subtoken() == unit_record->coast_id.province_index) {
                        order_result = TOKEN_ORDER_NOTE_FAR;
                    }
                }
            }

            if ((check_orders_on_submission) && (order_result == TOKEN_ORDER_NOTE_MBV)) {
                if (!can_move_to_province(convoying_unit, convoy_destination.get_subtoken())) {
                    order_result = TOKEN_ORDER_NOTE_FAR;
                }
            }

            if (order_result == TOKEN_ORDER_NOTE_MBV) {
                unit_record->order_type = MOVE_BY_CONVOY_ORDER;
                unit_record->move_dest.province_index = convoy_destination.get_subtoken();
                unit_record->move_dest.coast_token = TOKEN_UNIT_AMY;

                unit_record->convoy_step_list.clear();

                for (step_counter = 0; step_counter < convoy_via_list.get_message_length(); step_counter++) {
                    convoying_province = convoy_via_list.get_token(step_counter).get_subtoken();

                    unit_record->convoy_step_list.push_back(convoying_province);
                }
            }
        } else if (order_token == TOKEN_ORDER_RTO) {
            destination = get_coast_id(order.get_submessage(2), unit_record->unit_type);

            if ((check_orders_on_submission)
                && !can_move_to(unit_record, destination)) {
                order_result = TOKEN_ORDER_NOTE_FAR;
            } else if ((check_orders_on_submission)
                       && (unit_record->retreat_options.find(destination) == unit_record->retreat_options.end())) {
                order_result = TOKEN_ORDER_NOTE_NVR;
            } else {
                unit_record->order_type = RETREAT_ORDER;
                unit_record->move_dest = destination;
            }
        } else if (order_token == TOKEN_ORDER_DSB) {
            unit_record->order_type = DISBAND_ORDER;
        } else if (order_token == TOKEN_ORDER_BLD) {
            if (!winter_record->is_building
                || ((int) winter_record->builds_or_disbands.size() + winter_record->number_of_waives
                    >= winter_record->number_of_orders_required)) {
                order_result = TOKEN_ORDER_NOTE_NMB;
            } else {
                winter_order_unit = order.get_submessage(0);

                if (winter_order_unit.submessage_is_single_token(2)) {
                    build_location.province_index = winter_order_unit.get_token(2).get_subtoken();
                    build_location.coast_token = winter_order_unit.get_token(1);
                } else {
                    build_location.province_index = winter_order_unit.get_submessage(2).get_token(0).get_subtoken();
                    build_location.coast_token = winter_order_unit.get_submessage(2).get_token(1);
                }

                if (winter_order_unit.get_token(0).get_subtoken() != power_index) {
                    order_result = TOKEN_ORDER_NOTE_NYU;
                } else if (!game_map[build_location.province_index].is_supply_centre) {
                    order_result = TOKEN_ORDER_NOTE_NSC;
                } else if (game_map[build_location.province_index].home_centre_set.find(power_index)
                           == game_map[build_location.province_index].home_centre_set.end()) {
                    order_result = TOKEN_ORDER_NOTE_HSC;
                } else if (game_map[build_location.province_index].owner.get_subtoken() != power_index) {
                    order_result = TOKEN_ORDER_NOTE_YSC;
                } else if (units.find(build_location.province_index) != units.end()) {
                    order_result = TOKEN_ORDER_NOTE_ESC;
                } else if (game_map[build_location.province_index].coast_info.find(build_location.coast_token)
                           == game_map[build_location.province_index].coast_info.end()) {
                    order_result = TOKEN_ORDER_NOTE_CST;
                } else {
                    build_province.province_index = build_location.province_index;
                    build_province.coast_token = 0;

                    match_iterator = winter_record->builds_or_disbands.lower_bound(build_province);

                    if ((check_orders_on_submission)
                        && (match_iterator != winter_record->builds_or_disbands.end())
                        && (match_iterator->first.province_index == build_province.province_index)) {
                        order_result = TOKEN_ORDER_NOTE_ESC;
                    } else {
                        // Legal build
                        winter_record->builds_or_disbands.insert(
                                BUILDS_OR_DISBANDS::value_type(build_location, TOKEN_ORDER_NOTE_MBV));
                    }
                }
            }
        } else if (order_token == TOKEN_ORDER_REM) {
            if ((winter_record->is_building)
                || ((int) winter_record->builds_or_disbands.size() >= winter_record->number_of_orders_required)) {
                order_result = TOKEN_ORDER_NOTE_NMR;
            } else {
                unit_record = find_unit(order.get_submessage(0), units);

                if (unit_record == NULL) {
                    order_result = TOKEN_ORDER_NOTE_NSU;
                } else if (unit_record->nationality != power_index) {
                    order_result = TOKEN_ORDER_NOTE_NYU;
                } else {
                    // Legal removal
                    winter_record->builds_or_disbands.insert(
                            BUILDS_OR_DISBANDS::value_type(unit_record->coast_id, TOKEN_ORDER_NOTE_MBV));
                }
            }
        } else if (order_token == TOKEN_ORDER_WVE) {
            if (!winter_record->is_building
                || ((int) winter_record->builds_or_disbands.size() + winter_record->number_of_waives
                    >= winter_record->number_of_orders_required)) {
                order_result = TOKEN_ORDER_NOTE_NMB;
            } else if (order.get_token(0).get_subtoken() != power_index) {
                order_result = TOKEN_ORDER_NOTE_NYU;
            } else {
                winter_record->number_of_waives++;
            }
        }
    }

    return order_result;
}

MapAndUnits::UNIT_AND_ORDER *MapAndUnits::find_unit(TokenMessage &unit_to_find, UNITS &units_map) {
    UNIT_AND_ORDER *found_unit = NULL;
    UNITS::iterator unit_iterator;
    bool error = false;
    TokenMessage nationality;
    TokenMessage unit_type;
    TokenMessage location;
    Token province_token;
    Token coast;

    if (unit_to_find.get_submessage_count() != 3) {
        error = true;
    } else {
        nationality = unit_to_find.get_submessage(0);
        unit_type = unit_to_find.get_submessage(1);
        location = unit_to_find.get_submessage(2);

        if (!nationality.is_single_token()
            || !unit_type.is_single_token()) {
            error = true;
        } else {
            if (location.is_single_token()) {
                province_token = location.get_token();
                coast = unit_type.get_token();
            } else {
                province_token = location.get_token(0);
                coast = location.get_token(1);
            }
        }
    }

    if (!error) {
        unit_iterator = units_map.find(province_token.get_subtoken());

        if (unit_iterator == units_map.end()) {
            error = true;
        } else {
            if ((province_token.get_subtoken() >= number_of_provinces)
                || (unit_iterator->second.coast_id.coast_token != coast)
                || (unit_iterator->second.nationality != nationality.get_token().get_subtoken())
                || (unit_iterator->second.unit_type != unit_type.get_token())) {
                error = true;
            }
        }
    }

    if (!error) {
        found_unit = &(unit_iterator->second);
    }

    return found_unit;
}

bool MapAndUnits::can_move_to(UNIT_AND_ORDER *unit, COAST_ID destination) {
    bool can_move = false;
    PROVINCE_COASTS::iterator coast_details;

    coast_details = game_map[unit->coast_id.province_index].coast_info.find(unit->coast_id.coast_token);

    if (coast_details != game_map[unit->coast_id.province_index].coast_info.end()) {
        if (coast_details->second.adjacent_coasts.find(destination) != coast_details->second.adjacent_coasts.end()) {
            can_move = true;
        }
    }

    return can_move;
}

bool MapAndUnits::can_move_to_province(UNIT_AND_ORDER *unit, int province_index) {
    bool can_move = false;
    PROVINCE_COASTS::iterator coast_details;
    COAST_SET *adjacencies;
    COAST_SET::iterator first_record;
    COAST_ID min_coast;

    coast_details = game_map[unit->coast_id.province_index].coast_info.find(unit->coast_id.coast_token);

    if (coast_details != game_map[unit->coast_id.province_index].coast_info.end()) {
        adjacencies = &(coast_details->second.adjacent_coasts);

        min_coast.province_index = province_index;
        min_coast.coast_token = 0;

        first_record = adjacencies->lower_bound(min_coast);

        if ((first_record != adjacencies->end())
            && (first_record->province_index == province_index)) {
            can_move = true;
        }
    }

    return can_move;
}

bool MapAndUnits::has_route_to_province(UNIT_AND_ORDER *unit, PROVINCE_INDEX province_index,
                                        PROVINCE_INDEX province_to_avoid) {
    bool has_route;                                    // Whether there is a route
    PROVINCE_SET checked_provinces;                    // Provinces which have already been checked
    PROVINCE_SET provinces_to_check;                // Provinces still to check
    PROVINCE_INDEX province_being_checked;            // Province currently being processed
    PROVINCE_COASTS::iterator coast_iterator;        // Iterator through the coasts of a province
    COAST_SET::iterator adjacent_iterator;        // Iterator through the adjacent coasts to a province

    // First check if it can move directly
    has_route = can_move_to_province(unit, province_index);

    // If not, check for convoy routes
    if (!has_route
        && (unit->unit_type == TOKEN_UNIT_AMY)
        && (game_map[province_index].is_land)) {
        // Mark the source province as checked
        checked_provinces.insert(unit->coast_id.province_index);

        // Add all the adjacent provinces to the province to check list
        for (coast_iterator = game_map[unit->coast_id.province_index].coast_info.begin();
             coast_iterator != game_map[unit->coast_id.province_index].coast_info.end();
             coast_iterator++) {
            for (adjacent_iterator = coast_iterator->second.adjacent_coasts.begin();
                 adjacent_iterator != coast_iterator->second.adjacent_coasts.end();
                 adjacent_iterator++) {
                provinces_to_check.insert(adjacent_iterator->province_index);
            }
        }

        // If there is a province to avoid, then mark it as already checked. This will stop
        // any routes from going through it. This is used to stop a fleet supporting a convoyed
        // move that must be convoyed by that fleet
        if (province_to_avoid != -1) {
            checked_provinces.insert(province_to_avoid);
        }

        // While there are provinces to check
        while (!provinces_to_check.empty()
               && !has_route) {
            // Get the first
            province_being_checked = *(provinces_to_check.begin());
            provinces_to_check.erase(provinces_to_check.begin());

            // Check we haven't done it already
            if (checked_provinces.find(province_being_checked) == checked_provinces.end()) {
                // Add it to those we've done
                checked_provinces.insert(province_being_checked);

                // If it is a land province then check if it is the destination
                if (game_map[province_being_checked].is_land) {
                    if (province_being_checked == province_index) {
                        has_route = true;
                    }
                } else {
                    // Sea province, so check if occupied. If it is then add all adjacent provinces
                    if (units.find(province_being_checked) != units.end()) {
                        // Add all the adjacent provinces to the province to check list
                        for (coast_iterator = game_map[province_being_checked].coast_info.begin();
                             coast_iterator != game_map[province_being_checked].coast_info.end();
                             coast_iterator++) {
                            for (adjacent_iterator = coast_iterator->second.adjacent_coasts.begin();
                                 adjacent_iterator != coast_iterator->second.adjacent_coasts.end();
                                 adjacent_iterator++) {
                                provinces_to_check.insert(adjacent_iterator->province_index);
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
    int number_of_results;

    if ((current_season == TOKEN_SEASON_SPR)
        || (current_season == TOKEN_SEASON_FAL)) {
        number_of_results = get_movement_results(ord_messages);
    } else if ((current_season == TOKEN_SEASON_SUM)
               || (current_season == TOKEN_SEASON_AUT)) {
        number_of_results = get_retreat_results(ord_messages);
    } else {
        number_of_results = get_adjustment_results(ord_messages);
    }

    return number_of_results;
}

int MapAndUnits::get_movement_results(TokenMessage ord_messages[]) {
    int unit_counter = 0;
    UNITS::iterator unit_iterator;
    UNIT_AND_ORDER *unit;

    for (unit_iterator = units.begin();
         unit_iterator != units.end();
         unit_iterator++) {
        unit = &(unit_iterator->second);

        ord_messages[unit_counter] = describe_movement_result(unit);

        unit_counter++;
    }

    return unit_counter;
}

TokenMessage MapAndUnits::describe_movement_result(UNIT_AND_ORDER *unit) {
    TokenMessage movement_result;
    TokenMessage order;
    TokenMessage result;
    UNIT_LIST::iterator convoy_step_iterator;
    TokenMessage convoy_via;

    movement_result = TOKEN_COMMAND_ORD + describe_turn();

    switch (unit->order_type) {
        case NO_ORDER:
        case HOLD_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_HLD;

            if (!unit->dislodged) {
                result = TOKEN_RESULT_SUC;
            }

            break;
        }

        case MOVE_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_MTO + describe_coast(unit->move_dest);

            if (unit->bounce) {
                result = TOKEN_RESULT_BNC;
            } else if (unit->illegal_order) {
                result = unit->illegal_reason;
            } else {
                result = TOKEN_RESULT_SUC;
            }

            break;
        }

        case SUPPORT_TO_HOLD_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_SUP
                    + describe_unit(&(units[unit->other_source_province]));

            if (unit->support_cut) {
                result = TOKEN_RESULT_CUT;
            } else if (unit->support_void) {
                result = TOKEN_RESULT_NSO;
            } else if (unit->illegal_order) {
                result = unit->illegal_reason;
            } else {
                result = TOKEN_RESULT_SUC;
            }

            break;
        }

        case SUPPORT_TO_MOVE_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_SUP
                    + describe_unit(&(units[unit->other_source_province])) + TOKEN_ORDER_MTO
                    + game_map[unit->other_dest_province].province_token;

            if (unit->support_cut) {
                result = TOKEN_RESULT_CUT;
            } else if (unit->support_void) {
                result = TOKEN_RESULT_NSO;
            } else if (unit->illegal_order) {
                result = unit->illegal_reason;
            } else {
                result = TOKEN_RESULT_SUC;
            }

            break;
        }

        case CONVOY_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_CVY
                    + describe_unit(&(units[unit->other_source_province])) + TOKEN_ORDER_CTO
                    + game_map[unit->other_dest_province].province_token;

            if (unit->no_army_to_convoy) {
                result = TOKEN_RESULT_NSO;
            } else if (unit->illegal_order) {
                result = unit->illegal_reason;
            } else if (!unit->dislodged) {
                result = TOKEN_RESULT_SUC;
            }

            break;
        }

        case MOVE_BY_CONVOY_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_CTO + describe_coast(unit->move_dest);

            for (convoy_step_iterator = unit->convoy_step_list.begin();
                 convoy_step_iterator != unit->convoy_step_list.end();
                 convoy_step_iterator++) {
                convoy_via = convoy_via + game_map[*convoy_step_iterator].province_token;
            }

            order = order + TOKEN_ORDER_VIA & convoy_via;

            if (unit->no_convoy) {
                result = TOKEN_RESULT_NSO;
            } else if (unit->convoy_broken) {
                result = TOKEN_RESULT_DSR;
            } else if (unit->bounce) {
                result = TOKEN_RESULT_BNC;
            } else if (unit->illegal_order) {
                result = unit->illegal_reason;
            } else {
                result = TOKEN_RESULT_SUC;
            }

            break;
        }
    }

    if (unit->dislodged) {
        result = result + TOKEN_RESULT_RET;
    }

    movement_result = movement_result & order & result;

    return movement_result;
}

TokenMessage MapAndUnits::describe_turn() {
    TokenMessage current_turn;
    Token current_year_token;

    current_year_token.set_number(current_year);

    current_turn = current_season + current_year_token;

    current_turn.enclose_this();

    return current_turn;
}

TokenMessage MapAndUnits::describe_unit(UNIT_AND_ORDER *unit) {
    TokenMessage unit_message;

    unit_message = Token(CATEGORY_POWER, unit->nationality)
                   + unit->unit_type
                   + describe_coast(unit->coast_id);

    unit_message.enclose_this();

    return unit_message;
}

int MapAndUnits::get_retreat_results(TokenMessage ord_messages[]) {
    int unit_counter = 0;
    UNITS::iterator unit_iterator;
    UNIT_AND_ORDER *unit;

    for (unit_iterator = dislodged_units.begin();
         unit_iterator != dislodged_units.end();
         unit_iterator++) {
        unit = &(unit_iterator->second);

        ord_messages[unit_counter] = describe_retreat_result(unit);

        unit_counter++;
    }

    return unit_counter;
}

TokenMessage MapAndUnits::describe_retreat_result(UNIT_AND_ORDER *unit) {
    TokenMessage retreat_result;
    TokenMessage order;
    TokenMessage result;

    retreat_result = TOKEN_COMMAND_ORD + describe_turn();

    switch (unit->order_type) {
        case NO_ORDER:
        case DISBAND_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_DSB;

            result = TOKEN_RESULT_SUC;

            break;
        }

        case RETREAT_ORDER: {
            order = describe_unit(unit) + TOKEN_ORDER_RTO + describe_coast(unit->move_dest);

            if (unit->bounce) {
                result = TOKEN_RESULT_BNC;
            } else if (unit->illegal_order) {
                result = unit->illegal_reason;
            } else {
                result = TOKEN_RESULT_SUC;
            }

            break;
        }
    }

    retreat_result = retreat_result & order & result;

    return retreat_result;
}

int MapAndUnits::get_adjustment_results(TokenMessage ord_messages[]) {
    POWER_INDEX power_counter;
    int order_counter = 0;
    WINTER_ORDERS_FOR_POWER *orders;
    BUILDS_OR_DISBANDS::iterator order_iterator;
    int waive_counter;

    for (power_counter = 0; power_counter < number_of_powers; power_counter++) {
        orders = &(winter_orders[power_counter]);

        for (order_iterator = orders->builds_or_disbands.begin();
             order_iterator != orders->builds_or_disbands.end();
             order_iterator++) {
            ord_messages[order_counter] = describe_build_result(power_counter, orders, order_iterator);

            order_counter++;
        }

        if (orders->is_building) {
            for (waive_counter = 0; waive_counter < orders->number_of_waives; waive_counter++) {
                ord_messages[order_counter] = describe_waive(power_counter);

                order_counter++;
            }
        }
    }

    return order_counter;
}

TokenMessage MapAndUnits::describe_build_result(
        POWER_INDEX power_counter,
        WINTER_ORDERS_FOR_POWER *orders,
        BUILDS_OR_DISBANDS::iterator order_iterator) {
    TokenMessage build_result_message;
    TokenMessage order;

    order = Token(CATEGORY_POWER, power_counter);

    if (order_iterator->first.coast_token == TOKEN_UNIT_AMY) {
        order = order + TOKEN_UNIT_AMY;
    } else {
        order = order + TOKEN_UNIT_FLT;
    }

    order = order + describe_coast(order_iterator->first);

    order.enclose_this();

    if (orders->is_building) {
        order = order + TOKEN_ORDER_BLD;
    } else {
        order = order + TOKEN_ORDER_REM;
    }

    build_result_message = TOKEN_COMMAND_ORD
                           + describe_turn()
                           & order
                           & TOKEN_RESULT_SUC;

    return build_result_message;
}

TokenMessage MapAndUnits::describe_waive(POWER_INDEX power_counter) {
    TokenMessage build_result_message;
    TokenMessage order;

    order = Token(CATEGORY_POWER, power_counter) + TOKEN_ORDER_WVE;

    build_result_message = TOKEN_COMMAND_ORD
                           + describe_turn()
                           & order
                           & TOKEN_RESULT_SUC;

    return build_result_message;
}

void MapAndUnits::get_unit_positions(TokenMessage *now_message) {
    UNITS::iterator unit_iterator;

    *now_message = TOKEN_COMMAND_NOW + describe_turn();

    for (unit_iterator = units.begin();
         unit_iterator != units.end();
         unit_iterator++) {
        *now_message = *now_message + describe_unit(&(unit_iterator->second));
    }

    for (unit_iterator = dislodged_units.begin();
         unit_iterator != dislodged_units.end();
         unit_iterator++) {
        *now_message = *now_message + describe_dislodged_unit(&(unit_iterator->second));
    }
}

TokenMessage MapAndUnits::describe_dislodged_unit(UNIT_AND_ORDER *unit) {
    TokenMessage unit_message;
    TokenMessage retreat_locations;
    COAST_SET::iterator retreat_iterator;

    unit_message = Token(CATEGORY_POWER, unit->nationality)
                   + unit->unit_type
                   + describe_coast(unit->coast_id)
                   + TOKEN_PARAMETER_MRT;

    for (retreat_iterator = unit->retreat_options.begin();
         retreat_iterator != unit->retreat_options.end();
         retreat_iterator++) {
        retreat_locations = retreat_locations + describe_coast(*retreat_iterator);
    }

    unit_message = (unit_message & retreat_locations).enclose();

    return unit_message;
}

void MapAndUnits::get_sc_ownerships(TokenMessage *sco_message) {
    TokenMessage power_owned_scs[MAX_POWERS];
    TokenMessage unowned_scs;
    int province_counter;
    int sc_owner;
    int power_counter;

    for (province_counter = 0; province_counter < number_of_provinces; province_counter++) {
        if (game_map[province_counter].is_supply_centre) {
            if (game_map[province_counter].owner == TOKEN_PARAMETER_UNO) {
                unowned_scs = unowned_scs + game_map[province_counter].province_token;
            } else {
                sc_owner = game_map[province_counter].owner.get_subtoken();

                power_owned_scs[sc_owner] = power_owned_scs[sc_owner]
                                            + game_map[province_counter].province_token;
            }
        }
    }

    *sco_message = TOKEN_COMMAND_SCO;

    for (power_counter = 0; power_counter < number_of_powers; power_counter++) {
        if (power_owned_scs[power_counter].get_message_length() != TokenMessage::NO_MESSAGE) {
            *sco_message = *sco_message & (Token(CATEGORY_POWER, power_counter)
                                           + power_owned_scs[power_counter]);
        }
    }

    if (unowned_scs.get_message_length() != TokenMessage::NO_MESSAGE) {
        *sco_message = *sco_message & (TOKEN_PARAMETER_UNO + unowned_scs);
    }
}

int MapAndUnits::get_centre_count(Token power) {
    int centre_count = 0;
    int province_counter;

    for (province_counter = 0; province_counter < number_of_provinces; province_counter++) {
        if (game_map[province_counter].is_supply_centre) {
            if (game_map[province_counter].owner == power) {
                centre_count++;
            }
        }
    }

    return centre_count;
}

int MapAndUnits::get_unit_count(Token power) {
    int unit_count = 0;
    UNITS::iterator unit_iterator;

    for (unit_iterator = units.begin();
         unit_iterator != units.end();
         unit_iterator++) {
        if (unit_iterator->second.nationality == power.get_subtoken()) {
            unit_count++;
        }
    }

    return unit_count;
}

bool MapAndUnits::check_if_all_orders_received(POWER_INDEX power_index) {
    UNITS::iterator unit_iterator;                // Iterator to step through the units
    WINTER_ORDERS::iterator winter_iterator;    // Iterator to step through the winter orders
    bool all_ordered = true;                    // Whether all units have been ordered

    if ((current_season == TOKEN_SEASON_SPR)
        || (current_season == TOKEN_SEASON_FAL)) {
        // Go through all units and determine if any haven't been ordered
        for (unit_iterator = units.begin();
             unit_iterator != units.end();
             unit_iterator++) {
            if ((unit_iterator->second.nationality == power_index)
                && (unit_iterator->second.order_type == NO_ORDER)) {
                all_ordered = false;
            }
        }
    } else if ((current_season == TOKEN_SEASON_SUM)
               || (current_season == TOKEN_SEASON_AUT)) {
        // Go through all dislodged units and determine if any haven't been ordered
        for (unit_iterator = dislodged_units.begin();
             unit_iterator != dislodged_units.end();
             unit_iterator++) {
            if ((unit_iterator->second.nationality == power_index)
                && (unit_iterator->second.order_type == NO_ORDER)) {
                all_ordered = false;
            }
        }
    } else if (current_season == TOKEN_SEASON_WIN) {
        // Find the record for this power
        winter_iterator = winter_orders.find(power_index);

        if (winter_iterator != winter_orders.end()) {
            if (winter_iterator->second.number_of_orders_required
                > ((int) winter_iterator->second.builds_or_disbands.size()
                   + winter_iterator->second.number_of_waives)) {
                all_ordered = false;
            }
        }
    }

    return all_ordered;
}

bool MapAndUnits::unorder_adjustment(TokenMessage &not_sub_message, int power_index) {
    bool order_valid = true;
    TokenMessage sub_message;
    TokenMessage order;
    TokenMessage order_token_message;
    Token order_token;
    WINTER_ORDERS_FOR_POWER *winter_record;
    TokenMessage winter_order_unit;
    COAST_ID build_location;
    BUILDS_OR_DISBANDS::iterator match_iterator;

    sub_message = not_sub_message.get_submessage(1);
    order = sub_message.get_submessage(1);

    order_token_message = order.get_submessage(1);

    order_token = order_token_message.get_token();

    if ((order_token.get_token() & ORDER_TURN_MASK) == ORDER_BUILD_TURN_CHECK) {
        if (current_season != TOKEN_SEASON_WIN) {
            order_valid = false;
        } else {
            winter_record = &(winter_orders[power_index]);
        }
    } else {
        order_valid = false;
    }

    if (order_valid) {
        if ((order_token == TOKEN_ORDER_BLD) || (order_token == TOKEN_ORDER_REM)) {
            if ((winter_record->is_building == true) ^ (order_token == TOKEN_ORDER_BLD)) {
                order_valid = false;
            } else {
                winter_order_unit = order.get_submessage(0);

                if (winter_order_unit.submessage_is_single_token(2)) {
                    build_location.province_index = winter_order_unit.get_token(2).get_subtoken();
                    build_location.coast_token = winter_order_unit.get_token(1);
                } else {
                    build_location.province_index = winter_order_unit.get_submessage(2).get_token(0).get_subtoken();
                    build_location.coast_token = winter_order_unit.get_submessage(2).get_token(1);
                }

                if (winter_order_unit.get_token(0).get_subtoken() != power_index) {
                    order_valid = false;
                } else {
                    match_iterator = winter_record->builds_or_disbands.find(build_location);

                    if (match_iterator != winter_record->builds_or_disbands.end()) {
                        // Found the matching build/removal. Delete it.
                        winter_record->builds_or_disbands.erase(match_iterator);
                    } else {
                        order_valid = false;
                    }
                }
            }
        } else if (order_token == TOKEN_ORDER_WVE) {
            if (!winter_record->is_building
                || (winter_record->number_of_waives == 0)) {
                order_valid = false;
            } else if (order.get_token(0).get_subtoken() != power_index) {
                order_valid = false;
            } else {
                winter_record->number_of_waives--;
            }
        }
    }

    return order_valid;
}

