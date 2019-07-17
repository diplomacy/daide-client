/**
 * Diplomacy AI Server - Part of the DAIDE project.
 *
 * Turn Adjudicator Code. Based on the DPTG Adjudication Algorithm.
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

using DAIDE::MapAndUnits;

void MapAndUnits::adjudicate() {
    if ((current_season == TOKEN_SEASON_SPR) || (current_season == TOKEN_SEASON_FAL)) {
        adjudicate_moves();
    } else if ((current_season == TOKEN_SEASON_SUM) || (current_season == TOKEN_SEASON_AUT)) {
        adjudicate_retreats();
    } else {
        adjudicate_builds();
    }
}

void MapAndUnits::adjudicate_moves() {
    bool changes_made {true};
    bool futile_convoys_checked {false};
    bool futile_and_indomtiable_convoys_checked {false};

    initialise_move_adjudication();

    if (check_orders_on_adjudication) {
        check_for_illegal_move_orders();
    }

    cancel_inconsistent_convoys();
    cancel_inconsistent_supports();
    direct_attacks_cut_support();
    build_support_lists();
    build_convoy_subversion_list();

    while (changes_made) {
        changes_made = resolve_attacks_on_non_subverted_convoys();

        if (!changes_made && !futile_convoys_checked) {
            changes_made = check_for_futile_convoys();
            futile_convoys_checked = true;
        }

        if (!changes_made && !futile_and_indomtiable_convoys_checked) {
            changes_made = check_for_indomitable_and_futile_convoys();
            futile_and_indomtiable_convoys_checked = true;
        }
    }

    resolve_circles_of_subversion();
    identify_rings_of_attack_and_head_to_head_battles();
    advance_rings_of_attack();
    resolve_unbalanced_head_to_head_battles();
    resolve_balanced_head_to_head_battles();
    fight_ordinary_battles();
}

void MapAndUnits::initialise_move_adjudication() {
    UNIT_AND_ORDER *unit {nullptr};

    // Clear all lists of units
    attacker_map.clear();
    supporting_units.clear();
    convoying_units.clear();
    convoyed_units.clear();
    convoy_subversions.clear();
    rings_of_attack.clear();
    balanced_head_to_heads.clear();
    unbalanced_head_to_heads.clear();
    bounce_locations.clear();

    // Set up units to start adjudicating
    for (auto &unit_itr : units) {
        unit = &(unit_itr.second);

        unit->order_type_copy = unit->order_type;
        unit->supports.clear();
        unit->no_of_supports_to_dislodge = 0;
        unit->is_support_to_dislodge = false;
        unit->no_convoy = false;
        unit->no_army_to_convoy = false;
        unit->convoy_broken = false;
        unit->support_void = false;
        unit->support_cut = false;
        unit->bounce = false;
        unit->dislodged = false;
        unit->unit_moves = false;
        unit->move_number = NO_MOVE_NUMBER;
        unit->illegal_order = false;

        // Add unit to set according to action type
        switch (unit->order_type) {

            // Add to the attacker map
            case MOVE_ORDER: {
                attacker_map.insert(ATTACKER_MAP::value_type(unit->move_dest.province_index, unit_itr.first));
                break;
            }

            case SUPPORT_TO_HOLD_ORDER:
            case SUPPORT_TO_MOVE_ORDER: {
                supporting_units.insert(unit_itr.first);
                break;
            }

            case CONVOY_ORDER: {
                convoying_units.insert(unit_itr.first);
                break;
            }

            case MOVE_BY_CONVOY_ORDER: {
                convoyed_units.insert(unit_itr.first);
                break;
            }

            default:
                break;
        }
    }
}

void MapAndUnits::check_for_illegal_move_orders() {
    UNIT_AND_ORDER *unit_record {nullptr};      // The record for the unit being ordered
    UNIT_AND_ORDER *supported_unit {nullptr};   // The record for the unit being supported
    UNIT_AND_ORDER *convoyed_unit {nullptr};    // The record for the unit being convoyed
    PROVINCE_INDEX previous_province;           // The previous province of the convoy chain
    UNIT_AND_ORDER *convoying_unit {nullptr};   // The unit providing the convoy

    for (auto unit_itr = units.begin(); unit_itr != units.end(); unit_itr++) {
        unit_record = &(unit_itr->second);

        switch (unit_record->order_type) {

            // No checks to make
            case HOLD_ORDER:
                break;

            case MOVE_ORDER:
                if (!can_move_to(unit_record, unit_record->move_dest)) {
                    unit_record->order_type_copy = HOLD_ORDER;
                    unit_record->illegal_order = true;
                    unit_record->illegal_reason = TOKEN_ORDER_NOTE_FAR;
                }
                break;

            case SUPPORT_TO_HOLD_ORDER:
                supported_unit = &(units[unit_record->other_source_province]);

                if (!can_move_to_province(unit_record, supported_unit->coast_id.province_index)) {
                    unit_record->order_type_copy = HOLD_ORDER;
                    unit_record->illegal_order = true;
                    unit_record->illegal_reason = TOKEN_ORDER_NOTE_FAR;

                // Check it isn't trying to support itself
                } else if (supported_unit->coast_id.province_index == unit_record->coast_id.province_index) {
                    unit_record->order_type_copy = HOLD_ORDER;
                    unit_record->illegal_order = true;
                    unit_record->illegal_reason = TOKEN_ORDER_NOTE_FAR;
                }
                break;

            case SUPPORT_TO_MOVE_ORDER:
                if (!can_move_to_province(unit_record, unit_record->other_dest_province)) {
                    unit_record->order_type_copy = HOLD_ORDER;
                    unit_record->illegal_order = true;
                    unit_record->illegal_reason = TOKEN_ORDER_NOTE_FAR;

                // Check it isn't trying to support itself
                } else if (supported_unit->coast_id.province_index == unit_record->coast_id.province_index) {
                    unit_record->order_type_copy = HOLD_ORDER;
                    unit_record->illegal_order = true;
                    unit_record->illegal_reason = TOKEN_ORDER_NOTE_FAR;
                }
                break;

            case CONVOY_ORDER:
                convoyed_unit = &(units[unit_record->other_source_province]);

                if (unit_record->unit_type != TOKEN_UNIT_FLT) {
                    unit_record->order_type_copy = HOLD_ORDER;
                    unit_record->illegal_order = true;
                    unit_record->illegal_reason = TOKEN_ORDER_NOTE_NSF;
                } else if (game_map[unit_record->coast_id.province_index].is_land) {
                    unit_record->order_type_copy = HOLD_ORDER;
                    unit_record->illegal_order = true;
                    unit_record->illegal_reason = TOKEN_ORDER_NOTE_NAS;
                } else if (convoyed_unit->unit_type != TOKEN_UNIT_AMY) {
                    unit_record->order_type_copy = HOLD_ORDER;
                    unit_record->illegal_order = true;
                    unit_record->illegal_reason = TOKEN_ORDER_NOTE_NSA;
                }

                break;

            case MOVE_BY_CONVOY_ORDER:

                // Armies can't move by convoy
                if (unit_record->unit_type != TOKEN_UNIT_AMY) {
                    unit_record->order_type_copy = HOLD_ORDER;
                    unit_record->illegal_order = true;
                    unit_record->illegal_reason = TOKEN_ORDER_NOTE_NSA;

                // Fleet is convoying
                } else {
                    previous_province = unit_record->coast_id.province_index;

                    // Iterating through steps of convoy
                    for (auto convoy_step_itr = unit_record->convoy_step_list.begin();
                         convoy_step_itr != unit_record->convoy_step_list.end();
                         convoy_step_itr++) {

                        auto convoying_unit_itr = units.find(*convoy_step_itr);
                        if (convoying_unit_itr == units.end()) {
                            convoying_unit = nullptr;
                        } else {
                            convoying_unit = &(convoying_unit_itr->second);
                        }

                        if (convoying_unit == nullptr) {
                            unit_record->order_type_copy = HOLD_ORDER;
                            unit_record->illegal_order = true;
                            unit_record->illegal_reason = TOKEN_ORDER_NOTE_NSF;
                        } else if (game_map[convoying_unit->coast_id.province_index].is_land) {
                            unit_record->order_type_copy = HOLD_ORDER;
                            unit_record->illegal_order = true;
                            unit_record->illegal_reason = TOKEN_ORDER_NOTE_NAS;
                        } else if (!can_move_to_province(convoying_unit, previous_province)) {
                            unit_record->order_type_copy = HOLD_ORDER;
                            unit_record->illegal_order = true;
                            unit_record->illegal_reason = TOKEN_ORDER_NOTE_FAR;
                        }
                    }
                }

                if (!unit_record->illegal_order) {
                    if (!can_move_to_province(convoying_unit, unit_record->other_dest_province)) {
                        unit_record->order_type_copy = HOLD_ORDER;
                        unit_record->illegal_order = true;
                        unit_record->illegal_reason = TOKEN_ORDER_NOTE_FAR;
                    }
                }

                if (!unit_record->illegal_order) {
                    if (unit_record->move_dest.province_index == unit_record->coast_id.province_index) {
                        unit_record->order_type_copy = HOLD_ORDER;
                        unit_record->illegal_order = true;
                        unit_record->illegal_reason = TOKEN_ORDER_NOTE_FAR;
                    }
                }
                break;

            // Otherwise, it becomes a hold order
            default:
                unit_record->order_type_copy = HOLD_ORDER;
                break;
        }
    }
}

void MapAndUnits::cancel_inconsistent_convoys() {
    UNIT_AND_ORDER *convoyed_unit {nullptr};
    UNIT_AND_ORDER *convoying_unit {nullptr};
    bool order_ok;

    // For all armies moving by convoy, check all required fleets are ordered to convoy it
    auto convoyed_unit_itr = convoyed_units.begin();
    while (convoyed_unit_itr != convoyed_units.end()) {
        order_ok = true;
        convoyed_unit = &(units[*convoyed_unit_itr]);

        // Iterating through all steps
        for (auto convoying_step_itr = convoyed_unit->convoy_step_list.begin();
             convoying_step_itr != convoyed_unit->convoy_step_list.end();
             convoying_step_itr++) {

            auto unit_itr = units.find(*convoying_step_itr);
            if (unit_itr == units.end()) {
                order_ok = false;
            } else {
                convoying_unit = &(unit_itr->second);

                if ((convoying_unit->order_type_copy != CONVOY_ORDER)
                    || (convoying_unit->other_source_province != convoyed_unit->coast_id.province_index)
                    || (convoying_unit->other_dest_province != convoyed_unit->move_dest.province_index)) {
                    order_ok = false;
                }
            }
        }

        if (!order_ok) {
            convoyed_unit->order_type_copy = HOLD_NO_SUPPORT_ORDER;
            convoyed_unit->no_convoy = true;
            convoyed_unit_itr = convoyed_units.erase(convoyed_unit_itr);
        } else {
            convoyed_unit_itr++;
        }
    }

    // For all convoying fleets, check army is ordered to make use of convoy
    auto convoying_unit_itr = convoying_units.begin();
    while (convoying_unit_itr != convoying_units.end()) {
        order_ok = true;
        convoying_unit = &(units[*convoying_unit_itr]);

        auto unit_itr = units.find(convoying_unit->other_source_province);
        if (unit_itr == units.end()) {
            convoying_unit->no_army_to_convoy = true;
            order_ok = false;
        } else {
            convoyed_unit = &(unit_itr->second);

            if ((convoyed_unit->order_type != MOVE_BY_CONVOY_ORDER)
                || (convoyed_unit->coast_id.province_index != convoying_unit->other_source_province)
                || (convoyed_unit->move_dest.province_index != convoying_unit->other_dest_province)) {
                convoying_unit->no_army_to_convoy = true;
                order_ok = false;
            } else if (convoyed_unit->order_type_copy != MOVE_BY_CONVOY_ORDER) {
                // Army was ordered to convoy, but other fleets failed to make up the chain.
                order_ok = false;
            }
        }

        if (!order_ok) {
            convoying_unit->no_army_to_convoy = true;
            convoying_unit->order_type_copy = HOLD_ORDER;
            convoying_unit_itr = convoying_units.erase(convoying_unit_itr);
        } else {
            convoying_unit_itr++;
        }
    }

    // We have now finished with the convoying fleet set. All further work on
    // convoying fleets is done through the convoy step list for the army
    // No need to keep it up to date for the rest of the adjudicator
}

void MapAndUnits::cancel_inconsistent_supports() {
    bool order_ok {true};
    UNIT_AND_ORDER *supporting_unit {nullptr};
    UNIT_AND_ORDER *supported_unit {nullptr};

    // Check that for all supports to hold, supported unit is not moving
    // Check that for all supports to move, supported unit is moving correctly
    auto supporting_unit_itr = supporting_units.begin();
    while (supporting_unit_itr != supporting_units.end()) {
        order_ok = true;
        supporting_unit = &(units[*supporting_unit_itr]);

        auto supported_unit_itr = units.find(supporting_unit->other_source_province);
        if (supported_unit_itr == units.end()) {
            order_ok = false;
            supporting_unit->support_void = true;

        } else {
            supported_unit = &(supported_unit_itr->second);

            if (supporting_unit->order_type_copy == SUPPORT_TO_HOLD_ORDER) {
                if ((supported_unit->order_type_copy == MOVE_ORDER)
                    || (supported_unit->order_type_copy == MOVE_BY_CONVOY_ORDER)
                    || (supported_unit->order_type_copy == HOLD_NO_SUPPORT_ORDER)) {
                    order_ok = false;
                    supporting_unit->support_void = true;
                }
            } else {    // Support to move order
                if (((supported_unit->order_type != MOVE_ORDER)
                     && (supported_unit->order_type != MOVE_BY_CONVOY_ORDER))
                    || (supported_unit->move_dest.province_index != supporting_unit->other_dest_province)) {
                    order_ok = false;
                    supporting_unit->support_void = true;
                } else if ((supported_unit->order_type_copy != MOVE_ORDER)
                           && (supported_unit->order_type_copy != MOVE_BY_CONVOY_ORDER)) {
                    // Unit was ordered to move correctly, but move already failed
                    order_ok = false;
                }
            }
        }

        if (!order_ok) {
            supporting_unit->order_type_copy = HOLD_ORDER;
            supporting_unit_itr = supporting_units.erase(supporting_unit_itr);
        } else {
            supporting_unit_itr++;
        }
    }
}

void MapAndUnits::direct_attacks_cut_support() {
    UNIT_AND_ORDER *moving_unit {nullptr};
    UNIT_AND_ORDER *attacked_unit {nullptr};

    // For each moving unit, find the unit it is attacking
    for (auto &moving_unit_itr : attacker_map) {
        moving_unit = &(units[moving_unit_itr.second]);
        auto attacked_unit_itr = units.find(moving_unit->move_dest.province_index);

        if (attacked_unit_itr != units.end()) {
            attacked_unit = &(attacked_unit_itr->second);

            // If there is an attacked unit, and it is supporting, then consider cutting support
            if (attacked_unit->nationality != moving_unit->nationality) {
                if ((attacked_unit->order_type_copy == SUPPORT_TO_HOLD_ORDER)
                    || ((attacked_unit->order_type_copy == SUPPORT_TO_MOVE_ORDER)
                        && (attacked_unit->other_dest_province != moving_unit->coast_id.province_index))) {
                    // Support is cut
                    attacked_unit->support_cut = true;
                    attacked_unit->order_type_copy = HOLD_ORDER;
                    supporting_units.erase(attacked_unit->coast_id.province_index);
                }
            }
        }
    }
}

void MapAndUnits::build_support_lists() {
    UNIT_AND_ORDER *supporting_unit {nullptr};
    UNIT_AND_ORDER *supported_unit {nullptr};
    UNIT_AND_ORDER *attacked_unit {nullptr};

    // For each supporting unit, add it to the set of supports for the unit it is supporting
    for (int supporting_unit_itr : supporting_units) {

        supporting_unit = &(units[supporting_unit_itr]);
        supported_unit = &(units[supporting_unit->other_source_province]);
        supported_unit->supports.insert(supporting_unit_itr);

        // Check if the support is valid for dislodgement
        if (supporting_unit->order_type_copy == SUPPORT_TO_MOVE_ORDER) {
            auto attacked_unit_itr = units.find(supporting_unit->other_dest_province);

            if (attacked_unit_itr == units.end()) {
                supporting_unit->is_support_to_dislodge = true;
                supported_unit->no_of_supports_to_dislodge++;
            } else {
                attacked_unit = &(attacked_unit_itr->second);

                if ((supporting_unit->nationality != attacked_unit->nationality)
                    && (supported_unit->nationality != attacked_unit->nationality)) {
                    supporting_unit->is_support_to_dislodge = true;
                    supported_unit->no_of_supports_to_dislodge++;
                }
            }
        }
    }

    // We have now finished with the supporting units set. All further work on
    // supporting units is done through the unit they are supporting
    // No need to keep it up to date for the rest of the adjudicator
}

void MapAndUnits::build_convoy_subversion_list() {
    CONVOY_SUBVERSION convoy_subversion;
    UNIT_AND_ORDER *convoyed_unit {nullptr};
    UNIT_AND_ORDER *attacked_unit {nullptr};
    UNIT_AND_ORDER *supported_unit {nullptr};
    UNIT_AND_ORDER *support_against_unit {nullptr};

    // Build the list of convoys and check if each one subverts another
    for (int convoyed_moves_itr : convoyed_units) {

        // Create a convoy subversion record
        convoy_subversion.subverted_convoy_army = DOES_NOT_SUBVERT;
        convoy_subversion.number_of_subversions = 0;
        convoy_subversion.subversion_type = NOT_SUBVERTED_CONVOY;

        // Check if this convoy subverts another
        convoyed_unit = &(units[convoyed_moves_itr]);
        auto attacked_unit_itr = units.find(convoyed_unit->move_dest.province_index);

        if (attacked_unit_itr != units.end()) {
            attacked_unit = &(attacked_unit_itr->second);

            if (attacked_unit->nationality != convoyed_unit->nationality) {
                if (attacked_unit->order_type_copy == SUPPORT_TO_HOLD_ORDER) {
                    supported_unit = &(units[attacked_unit->other_source_province]);

                    // We have subversion
                    if (supported_unit->order_type_copy == CONVOY_ORDER) {
                        convoy_subversion.subverted_convoy_army = supported_unit->other_source_province;
                    }

                } else if (attacked_unit->order_type_copy == SUPPORT_TO_MOVE_ORDER) {
                    auto support_against_itr = units.find(attacked_unit->other_dest_province);

                    if (support_against_itr != units.end()) {
                        support_against_unit = &(support_against_itr->second);

                        // We have subversion
                        if (support_against_unit->order_type_copy == CONVOY_ORDER) {
                            convoy_subversion.subverted_convoy_army = support_against_unit->other_source_province;
                        }
                    }
                }
            }
        }
        convoy_subversions[convoyed_moves_itr] = convoy_subversion;
    }

    // Mark subverted convoys as such
    for (auto convoy_subversion_itr = convoy_subversions.begin();
         convoy_subversion_itr != convoy_subversions.end();
         convoy_subversion_itr++) {

        if (convoy_subversion_itr->second.subverted_convoy_army != DOES_NOT_SUBVERT) {
            auto subverted_convoy_itr = convoy_subversions.find(convoy_subversion_itr->second.subverted_convoy_army);
            subverted_convoy_itr->second.subversion_type = SUBVERTED_CONVOY;
            subverted_convoy_itr->second.number_of_subversions++;
        }
    }

    // We have now finished with the convoyed army set. All further work on
    // convoyed armies is done through the subversion map
    // No need to keep it up to date for the rest of the adjudicator
}

bool MapAndUnits::resolve_attacks_on_non_subverted_convoys() {
    bool changes_made {false};
    bool unit_dislodged {false};
    bool convoy_broken {false};
    UNIT_AND_ORDER *convoyed_army {nullptr};
    UNIT_AND_ORDER *convoying_fleet {nullptr};
    CONVOY_SUBVERSION *subverted_convoy {nullptr};

    // For each convoy, check if it is subverted
    auto convoy_subversion_itr = convoy_subversions.begin();

    while (convoy_subversion_itr != convoy_subversions.end()) {
        if (convoy_subversion_itr->second.subversion_type == NOT_SUBVERTED_CONVOY) {

            // If not, resolve attacks on its fleets
            convoyed_army = &(units[convoy_subversion_itr->first]);
            convoy_broken = false;

            for (int &convoying_fleet_itr : convoyed_army->convoy_step_list) {
                unit_dislodged = resolve_attacks_on_occupied_province(convoying_fleet_itr);

                if (unit_dislodged) {
                    convoy_broken = true;
                }
            }

            // If the convoy is broken, then revert all units involved to hold
            if (convoy_broken) {
                for (int &convoying_fleet_itr : convoyed_army->convoy_step_list) {
                    convoying_fleet = &(units[convoying_fleet_itr]);
                    convoying_fleet->order_type_copy = HOLD_ORDER;
                }

                convoyed_army->order_type_copy = HOLD_NO_SUPPORT_ORDER;
                convoyed_army->convoy_broken = true;

                // All supports for this army are now invalid
                convoyed_army->supports.clear();
                convoyed_army->no_of_supports_to_dislodge = 0;

            } else {
                // Convoy is not broken, so cut any support it is attacking
                cut_support(convoyed_army->move_dest.province_index);

                // Add convoyed attack to list of attacks on destination province
                attacker_map.insert(ATTACKER_MAP::value_type(convoyed_army->move_dest.province_index,
                                                             convoyed_army->coast_id.province_index));
            }

            // If the convoy was subverting another, then it doesn't any more
            if (convoy_subversion_itr->second.subverted_convoy_army != DOES_NOT_SUBVERT) {
                subverted_convoy = &(convoy_subversions[convoy_subversion_itr->second.subverted_convoy_army]);
                subverted_convoy->number_of_subversions--;

                if (subverted_convoy->number_of_subversions == 0) {
                    subverted_convoy->subversion_type = NOT_SUBVERTED_CONVOY;
                }
            }

            // Remove the convoy from the convoy subversion records
            convoy_subversion_itr = convoy_subversions.erase(convoy_subversion_itr);
            changes_made = true;

        } else {
            // Move onto the next convoy subversion record
            convoy_subversion_itr++;
        }
    }

    return changes_made;
}

bool MapAndUnits::check_for_futile_convoys() {
    bool changes_made {false};
    bool unit_dislodged {false};
    bool convoy_broken {false};
    PROVINCE_INDEX convoyed_army_province {-1};
    PROVINCE_INDEX subverted_province {-1};
    PROVINCE_INDEX subverted_convoy_army_index {-1};
    UNIT_AND_ORDER *convoyed_army {nullptr};
    UNIT_AND_ORDER *attacked_unit {nullptr};
    UNIT_AND_ORDER *subverted_convoy_army {nullptr};
    UNIT_AND_ORDER *convoying_fleet {nullptr};
    CONVOY_SUBVERSION *broken_convoy_subversion_record {nullptr};
    CONVOY_SUBVERSION *sub_subverted_convoy {nullptr};

    // For each convoy, find the convoy it subverts, and see if we can
    // resolve that convoy by checking the other fleets for dislodgement
    auto convoy_subversion_itr = convoy_subversions.begin();

    while (convoy_subversion_itr != convoy_subversions.end()) {
        convoyed_army_province = convoy_subversion_itr->first;

        if (convoy_subversion_itr->second.subverted_convoy_army != DOES_NOT_SUBVERT) {
            convoyed_army = &(units[convoyed_army_province]);
            attacked_unit = &(units[convoyed_army->move_dest.province_index]);

            if (attacked_unit->order_type_copy == SUPPORT_TO_HOLD_ORDER) {
                subverted_province = attacked_unit->other_source_province;
            } else {
                subverted_province = attacked_unit->other_dest_province;
            }

            subverted_convoy_army_index = convoy_subversion_itr->second.subverted_convoy_army;
            subverted_convoy_army = &(units[subverted_convoy_army_index]);
            convoy_broken = false;

            // Go through the convoy and check every fleet except the subverted one
            for (int & convoying_fleet_itr : subverted_convoy_army->convoy_step_list) {
                if (convoying_fleet_itr != subverted_province) {
                    unit_dislodged = resolve_attacks_on_occupied_province(convoying_fleet_itr);
                    if (unit_dislodged) {
                        convoy_broken = true;
                    }
                }
            }

            // If the convoy is broken, then revert all units involved to hold
            if (convoy_broken) {
                for (int & convoying_fleet_itr : subverted_convoy_army->convoy_step_list) {
                    convoying_fleet = &(units[convoying_fleet_itr]);
                    convoying_fleet->order_type_copy = HOLD_ORDER;
                }

                subverted_convoy_army->order_type_copy = HOLD_NO_SUPPORT_ORDER;
                subverted_convoy_army->convoy_broken = true;

                // All supports for this army are now invalid
                subverted_convoy_army->supports.clear();
                subverted_convoy_army->no_of_supports_to_dislodge = 0;

                // Find the convoy this convoy was subverting. It is no longer subverted.
                broken_convoy_subversion_record = &(convoy_subversions[subverted_convoy_army_index]);
                auto sub_subverted_convoy_itr = convoy_subversions.find(
                        broken_convoy_subversion_record->subverted_convoy_army);
                sub_subverted_convoy = &(sub_subverted_convoy_itr->second);

                sub_subverted_convoy->number_of_subversions = 0;
                sub_subverted_convoy->subversion_type = NOT_SUBVERTED_CONVOY;

                // The convoy that was subverting this one, no longer has a convoy to subvert
                convoy_subversion_itr->second.subverted_convoy_army = DOES_NOT_SUBVERT;

                // Remove the convoy from the convoy subversion records
                convoy_subversions.erase(convoy_subversion_itr);
                changes_made = true;
            }
        }
        convoy_subversion_itr = convoy_subversions.upper_bound(convoyed_army_province);
    }

    return changes_made;
}

bool MapAndUnits::check_for_indomitable_and_futile_convoys() {
    bool changes_made {false};
    PROVINCE_INDEX convoyed_army_province {-1};
    PROVINCE_INDEX subverted_province {-1};
    PROVINCE_INDEX subverted_convoy_army_index {-1};
    PROVINCE_INDEX dislodging_unit_if_not_cut {-1};
    PROVINCE_INDEX dislodging_unit_if_cut {-1};
    UNIT_AND_ORDER *convoyed_army {nullptr};
    UNIT_AND_ORDER *attacked_unit {nullptr};
    UNIT_AND_ORDER *supported_fleet {nullptr};
    UNIT_AND_ORDER *subverted_convoy_army {nullptr};
    CONVOY_SUBVERSION *subverted_convoy_record {nullptr};
    UNIT_AND_ORDER *convoying_fleet {nullptr};
    CONVOY_SUBVERSION *broken_convoy_subversion_record {nullptr};
    CONVOY_SUBVERSION *sub_subverted_convoy {nullptr};

    // For each convoy, check the subverted fleet to determine if the convoy
    // is indomitable, futile, or confused
    auto convoy_subversion_itr = convoy_subversions.begin();

    while (convoy_subversion_itr != convoy_subversions.end()) {
        convoyed_army_province = convoy_subversion_itr->first;

        if (convoy_subversion_itr->second.subverted_convoy_army != DOES_NOT_SUBVERT) {
            convoyed_army = &(units[convoyed_army_province]);
            attacked_unit = &(units[convoyed_army->move_dest.province_index]);

            if (attacked_unit->order_type_copy == SUPPORT_TO_HOLD_ORDER) {
                subverted_province = attacked_unit->other_source_province;
                supported_fleet = &(units[subverted_province]);
            } else {
                subverted_province = attacked_unit->other_dest_province;
                supported_fleet = &(units[attacked_unit->other_source_province]);
            }

            subverted_convoy_army_index = convoy_subversion_itr->second.subverted_convoy_army;
            subverted_convoy_army = &(units[subverted_convoy_army_index]);
            subverted_convoy_record = &(convoy_subversions[subverted_convoy_army_index]);

            // Find the dislodging unit with support in tact
            dislodging_unit_if_not_cut = find_dislodging_unit(subverted_province);

            // Temporarily remove the support of the attacked unit
            supported_fleet->supports.erase(attacked_unit->coast_id.province_index);
            if (attacked_unit->is_support_to_dislodge) {
                supported_fleet->no_of_supports_to_dislodge--;
            }

            // Find the dislodging unit with support cut
            dislodging_unit_if_cut = find_dislodging_unit(subverted_province);

            // Reinstate the support of the attacked unit
            supported_fleet->supports.insert(attacked_unit->coast_id.province_index);
            if (attacked_unit->is_support_to_dislodge) {
                supported_fleet->no_of_supports_to_dislodge++;
            }

            if (dislodging_unit_if_not_cut != NO_DISLODGING_UNIT) {
                if (dislodging_unit_if_cut != NO_DISLODGING_UNIT) {

                    // Convoy is futile
                    for (int & convoying_fleet_itr : subverted_convoy_army->convoy_step_list) {
                        convoying_fleet = &(units[convoying_fleet_itr]);
                        convoying_fleet->order_type_copy = HOLD_ORDER;
                    }

                    subverted_convoy_army->order_type_copy = HOLD_NO_SUPPORT_ORDER;
                    subverted_convoy_army->convoy_broken = true;

                    // All supports for this army are now invalid
                    subverted_convoy_army->supports.clear();
                    subverted_convoy_army->no_of_supports_to_dislodge = 0;

                    // Find the convoy this convoy was subverting. It is no longer subverted.
                    broken_convoy_subversion_record = &(convoy_subversions[subverted_convoy_army_index]);
                    auto sub_subverted_convoy_itr = convoy_subversions.find(
                            broken_convoy_subversion_record->subverted_convoy_army);
                    sub_subverted_convoy = &(sub_subverted_convoy_itr->second);

                    sub_subverted_convoy->number_of_subversions = 0;
                    sub_subverted_convoy->subversion_type = NOT_SUBVERTED_CONVOY;

                    // The convoy that was subverting this one, no longer has a convoy to subvert
                    convoy_subversion_itr->second.subverted_convoy_army = DOES_NOT_SUBVERT;

                    // Remove the convoy from the convoy subversion records
                    convoy_subversions.erase(convoy_subversion_itr);
                    changes_made = true;
                } else {
                    // Convoy is subverted
                    // No change
                }

            } else {
                // Convoy is confused
                if (dislodging_unit_if_cut != NO_DISLODGING_UNIT) {
                    subverted_convoy_record->subversion_type = CONFUSED_CONVOY;

                // Convoy is indomitable
                } else {
                    // Convoy is not broken, so cut any support it is attacking
                    cut_support(subverted_convoy_army->move_dest.province_index);

                    // Add convoyed attack to list of attacks on destination province
                    attacker_map.insert(ATTACKER_MAP::value_type(subverted_convoy_army->move_dest.province_index,
                                                                 subverted_convoy_army->coast_id.province_index));

                    // Find the convoy this convoy was subverting. It is no longer subverted.
                    broken_convoy_subversion_record = &(convoy_subversions[subverted_convoy_army_index]);
                    auto sub_subverted_convoy_itr = convoy_subversions.find(
                            broken_convoy_subversion_record->subverted_convoy_army);
                    sub_subverted_convoy = &(sub_subverted_convoy_itr->second);

                    sub_subverted_convoy->number_of_subversions = 0;
                    sub_subverted_convoy->subversion_type = NOT_SUBVERTED_CONVOY;

                    // The convoy that was subverting this one, no longer has a convoy to subvert
                    convoy_subversion_itr->second.subverted_convoy_army = DOES_NOT_SUBVERT;

                    // Remove the convoy from the convoy subversion records
                    convoy_subversions.erase(convoy_subversion_itr);
                    changes_made = true;
                }
            }
        }
        convoy_subversion_itr = convoy_subversions.upper_bound(convoyed_army_province);
    }

    return changes_made;
}

void MapAndUnits::resolve_circles_of_subversion() {
    bool convoy_is_confused {false};
    PROVINCE_INDEX convoyed_army_province {-1};
    PROVINCE_INDEX next_convoyed_army {-1};
    UNIT_AND_ORDER *convoyed_army {nullptr};
    UNIT_AND_ORDER *convoying_fleet {nullptr};
    UNIT_AND_ORDER *attacking_unit {nullptr};

    auto convoy_subversion_itr = convoy_subversions.begin();

    // Continue until there are no records left in the convoy subversions list
    while (convoy_subversion_itr != convoy_subversions.end()) {

        // Go round the convoy loop and determine if any of the convoys are confused
        convoyed_army_province = convoy_subversion_itr->first;
        convoy_is_confused = false;

        do {
            if (convoy_subversion_itr->second.subversion_type == CONFUSED_CONVOY) {
                convoy_is_confused = true;
            }
            next_convoyed_army = convoy_subversion_itr->second.subverted_convoy_army;
            convoy_subversion_itr = convoy_subversions.find(next_convoyed_army);
        } while (next_convoyed_army != convoyed_army_province);

        // If any are confused
        if (convoy_is_confused) {

            // All attacks on the convoy fail
            convoy_subversion_itr = convoy_subversions.begin();

            do {
                convoyed_army = &(units[convoy_subversion_itr->first]);

                for (int &convoying_fleet_itr : convoyed_army->convoy_step_list) {

                    for (auto attacking_unit_itr = attacker_map.lower_bound(convoying_fleet_itr);
                         attacking_unit_itr != attacker_map.upper_bound(convoying_fleet_itr);
                         attacking_unit_itr++) {

                        // For all attacking units, reset to hold, and cancel all supports
                        attacking_unit = &(units[attacking_unit_itr->second]);

                        attacking_unit->order_type_copy = HOLD_NO_SUPPORT_ORDER;
                        attacking_unit->supports.clear();
                        attacking_unit->no_of_supports_to_dislodge = 0;
                        attacking_unit->bounce = true;
                    }

                    // Remove these attacks from the attacker map
                    attacker_map.erase(convoying_fleet_itr);
                }

                // Move to the next item in the chain
                next_convoyed_army = convoy_subversion_itr->second.subverted_convoy_army;
                convoy_subversion_itr = convoy_subversions.find(next_convoyed_army);

            } while (convoy_subversion_itr != convoy_subversions.begin());
        }

        // All convoys in the loop fail

        convoy_subversion_itr = convoy_subversions.begin();

        while (convoy_subversion_itr != convoy_subversions.end()) {
            convoyed_army = &(units[convoy_subversion_itr->first]);

            for (int & convoying_fleet_itr : convoyed_army->convoy_step_list) {
                convoying_fleet = &(units[convoying_fleet_itr]);
                convoying_fleet->order_type_copy = HOLD_ORDER;
            }

            convoyed_army->order_type_copy = HOLD_NO_SUPPORT_ORDER;
            convoyed_army->convoy_broken = true;

            // All supports for this army are now invalid
            convoyed_army->supports.clear();
            convoyed_army->no_of_supports_to_dislodge = 0;

            // Move to the next item in the chain, and delete this one
            next_convoyed_army = convoy_subversion_itr->second.subverted_convoy_army;
            convoy_subversions.erase(convoy_subversion_itr);
            convoy_subversion_itr = convoy_subversions.find(next_convoyed_army);
        }

        // No need to worry about dislodging the fleets of non-confused convoy loops here.
        // This will occur later in the algorithm anyway.

        // Find the new first record in the convoy subversions list
        convoy_subversion_itr = convoy_subversions.begin();
    }
}

void MapAndUnits::identify_rings_of_attack_and_head_to_head_battles() {
    bool chain_end_found {false};
    bool loop_found {false};
    int move_ctr {0};
    int chain_start {-1};
    int last_convoy {-1};
    UNIT_AND_ORDER *moving_unit {nullptr};
    UNIT_AND_ORDER *other_moving_unit {nullptr};

    // Go through each unit in the attacker map
    for (auto &moving_unit_itr : attacker_map) {
        moving_unit = &(units[moving_unit_itr.second]);
        chain_start = move_ctr;
        last_convoy = NO_MOVE_NUMBER;
        chain_end_found = false;
        loop_found = false;

        // Follow the chain of unit attacking another unit
        while (!chain_end_found) {
            // If we find a unit with a number, we've branched into a chain we've already considered
            if (moving_unit->move_number != NO_MOVE_NUMBER) {
                chain_end_found = true;

                // If it is this chain, we have a loop
                if (moving_unit->move_number >= chain_start) {
                    loop_found = true;
                }

            // If we find a non-moving unit, it is the end of the chain
            } else if ((moving_unit->order_type_copy != MOVE_ORDER)
                       && (moving_unit->order_type_copy != MOVE_BY_CONVOY_ORDER)) {
                chain_end_found = true;

            } else {
                // Number the move so we know which chain it is in
                moving_unit->move_number = move_ctr;

                if (moving_unit->order_type_copy == MOVE_BY_CONVOY_ORDER) {
                    last_convoy = move_ctr;
                }

                move_ctr++;

                // Find the next unit in the chain
                auto next_unit_itr = units.find(moving_unit->move_dest.province_index);

                // If there isn't one, chain ends
                if (next_unit_itr == units.end()) {
                    chain_end_found = true;
                } else {
                    moving_unit = &(next_unit_itr->second);
                }
            }
        }

        if (loop_found) {

            // Ring of attacks
            if ((move_ctr - moving_unit->move_number >= 3) || (last_convoy >= moving_unit->move_number)) {
                rings_of_attack.insert(moving_unit->coast_id.province_index);

            // Head to head. Determine if balanced or unbalanced
            } else {
                other_moving_unit = &(units[moving_unit->move_dest.province_index]);

                if (moving_unit->no_of_supports_to_dislodge > other_moving_unit->supports.size()) {
                    unbalanced_head_to_heads.insert(moving_unit->coast_id.province_index);
                } else if (other_moving_unit->no_of_supports_to_dislodge > moving_unit->supports.size()) {
                    unbalanced_head_to_heads.insert(other_moving_unit->coast_id.province_index);
                } else {
                    balanced_head_to_heads.insert(moving_unit->coast_id.province_index);
                }
            }
        }
    }
}

void MapAndUnits::advance_rings_of_attack() {
    int first_province {-1};
    const PROVINCE_INDEX NO_RING_BREAKER {-1};
    PROVINCE_INDEX ring_breaking_unit {-1};
    UNIT_LIST units_in_ring {};
    UNIT_LIST::iterator ring_breaking_unit_itr;
    UNIT_AND_ORDER *ring_unit {nullptr};

    // For each ring of attack
    for (int ring_set_itr : rings_of_attack) {

        // Build the list of units in the ring, in reverse order, and work out the status of each
        first_province = ring_set_itr;
        ring_unit = &(units[first_province]);

        units_in_ring.clear();
        ring_breaking_unit = NO_RING_BREAKER;

        do {
            units_in_ring.push_front(ring_unit->coast_id.province_index);

            ring_unit->ring_unit_status = determine_ring_status(ring_unit->move_dest.province_index,
                                                                ring_unit->coast_id.province_index);

            // If this unit can't advance, then it is the ring breaker
            if ((ring_unit->ring_unit_status != RING_ADVANCES_REGARDLESS)
                 && (ring_unit->ring_unit_status != RING_ADVANCES_IF_VACANT)) {

                ring_breaking_unit = ring_unit->coast_id.province_index;
                ring_breaking_unit_itr = units_in_ring.begin();
            }

            ring_unit = &(units[ring_unit->move_dest.province_index]);
        } while (ring_unit->coast_id.province_index != first_province);

        // Every unit in the ring advances
        if (ring_breaking_unit == NO_RING_BREAKER) {
            for (int & ring_itr : units_in_ring) {
                advance_unit(ring_itr);
            }

        // Check on the ring status of the ring breaker
        } else {
            ring_unit = &(units[ring_breaking_unit]);

            if (ring_unit->ring_unit_status == STANDOFF_REGARDLESS) {
                bounce_all_attacks_on_province(ring_unit->move_dest.province_index);
            } else if (ring_unit->ring_unit_status == SIDE_ADVANCES_REGARDLESS) {
                bounce_attack(ring_unit);

            // We don't know what happens in province this unit is moving into. Work backwards
            } else {
                ring_breaking_unit_itr++;

                if (ring_breaking_unit_itr == units_in_ring.end()) {
                    ring_breaking_unit_itr = units_in_ring.begin();
                }

                ring_unit = &(units[*ring_breaking_unit_itr]);

                // We know the unit ahead of this one is not moving. Determine what happens to this one
                if (ring_unit->ring_unit_status == SIDE_ADVANCES_REGARDLESS) {
                    bounce_attack(ring_unit);
                } else if (ring_unit->ring_unit_status != RING_ADVANCES_REGARDLESS) {
                    bounce_all_attacks_on_province(ring_unit->move_dest.province_index);

                // This unit will advance. Work backwards until we find one that won't
                } else {
                    do {
                        ring_breaking_unit_itr++;

                        if (ring_breaking_unit_itr == units_in_ring.end()) {
                            ring_breaking_unit_itr = units_in_ring.begin();
                        }

                        ring_unit = &(units[*ring_breaking_unit_itr]);

                        if ((ring_unit->ring_unit_status == SIDE_ADVANCES_REGARDLESS)
                            || (ring_unit->ring_unit_status == SIDE_ADVANCES_IF_VACANT)) {
                            bounce_attack(ring_unit);
                        } else if (ring_unit->ring_unit_status == STANDOFF_REGARDLESS) {
                            bounce_all_attacks_on_province(ring_unit->move_dest.province_index);
                        }
                    } while ((ring_unit->ring_unit_status == RING_ADVANCES_IF_VACANT)
                             || (ring_unit->ring_unit_status == RING_ADVANCES_REGARDLESS));
                }
            }
        }
    }
}

MapAndUnits::RING_UNIT_STATUS
MapAndUnits::determine_ring_status(PROVINCE_INDEX province, PROVINCE_INDEX ring_unit_source) {
    RING_UNIT_STATUS ring_status;
    PROVINCE_INDEX most_supported_unit {-1};
    int most_supports {-1};
    int most_supports_to_dislodge {-1};
    int second_most_supports {-1};
    UNIT_AND_ORDER *attacking_unit {nullptr};

    // Find the strength of the most supported and second most supported unit
    for (auto attacking_unit_itr = attacker_map.lower_bound(province);
         attacking_unit_itr != attacker_map.upper_bound(province);
         attacking_unit_itr++) {

        attacking_unit = &(units[attacking_unit_itr->second]);

        if (attacking_unit->supports.size() > most_supports) {
            second_most_supports = most_supports;
            most_supports = attacking_unit->supports.size();
            most_supports_to_dislodge = attacking_unit->no_of_supports_to_dislodge;
            most_supported_unit = attacking_unit_itr->second;
        } else if (attacking_unit->supports.size() > second_most_supports) {
            second_most_supports = attacking_unit->supports.size();
        }
    }

    // Determine the status of the ring province based on the two strongest supported units
    if (most_supports == second_most_supports) {
        ring_status = STANDOFF_REGARDLESS;
    } else if (most_supported_unit == ring_unit_source) {
        if ((most_supports_to_dislodge > 0) && (most_supports_to_dislodge > second_most_supports)) {
            ring_status = RING_ADVANCES_REGARDLESS;
        } else {
            ring_status = RING_ADVANCES_IF_VACANT;
        }
    } else {
        if ((most_supports_to_dislodge > 0) && (most_supports_to_dislodge > second_most_supports)) {
            ring_status = SIDE_ADVANCES_REGARDLESS;
        } else {
            ring_status = SIDE_ADVANCES_IF_VACANT;
        }
    }

    return ring_status;
}

void MapAndUnits::advance_unit(PROVINCE_INDEX unit_to_advance) {
    PROVINCE_INDEX attacked_province {-1};
    UNIT_AND_ORDER *moving_unit {nullptr};
    UNIT_AND_ORDER *bounced_unit {nullptr};

    // Advance the unit currently in the province given.
    moving_unit = &(units[unit_to_advance]);
    attacked_province = moving_unit->move_dest.province_index;
    moving_unit->unit_moves = true;

    // Cancel all other moves into the province it is moving into
    for (auto attacker_itr = attacker_map.lower_bound(attacked_province);
         attacker_itr != attacker_map.upper_bound(attacked_province);
         attacker_itr++) {

        bounced_unit = &(units[attacker_itr->second]);

        if (bounced_unit->coast_id.province_index != unit_to_advance) {
            bounced_unit->order_type_copy = HOLD_NO_SUPPORT_ORDER;
            bounced_unit->supports.clear();
            bounced_unit->no_of_supports_to_dislodge = 0;
            bounced_unit->bounce = true;
        }
    }
    attacker_map.erase(attacked_province);
}

void MapAndUnits::bounce_all_attacks_on_province(PROVINCE_INDEX province_index) {
    UNIT_AND_ORDER *bounced_unit {nullptr};

    // Bounce all the attacks on the given province.
    for (auto attacker_itr = attacker_map.lower_bound(province_index);
         attacker_itr != attacker_map.upper_bound(province_index);
         attacker_itr++) {

        bounced_unit = &(units[attacker_itr->second]);
        bounced_unit->order_type_copy = HOLD_NO_SUPPORT_ORDER;
        bounced_unit->supports.clear();
        bounced_unit->no_of_supports_to_dislodge = 0;
        bounced_unit->bounce = true;
    }

    // Remove them all from the attacker map
    attacker_map.erase(province_index);

    // Add to the list of provinces containing a bounce
    bounce_locations.insert(province_index);
}

void MapAndUnits::bounce_attack(UNIT_AND_ORDER *unit) {

    // Bounce the given unit out of the province it is attacking
    unit->order_type_copy = HOLD_NO_SUPPORT_ORDER;
    unit->supports.clear();
    unit->no_of_supports_to_dislodge = 0;
    unit->bounce = true;

    // Remove from attacker map
    auto attacker_itr = attacker_map.lower_bound(unit->move_dest.province_index);
    while (attacker_itr != attacker_map.upper_bound(unit->move_dest.province_index)) {
        if (attacker_itr->second == unit->coast_id.province_index) {
            attacker_itr = attacker_map.erase(attacker_itr);
        } else {
            attacker_itr++;
        }
    }
}

void MapAndUnits::resolve_unbalanced_head_to_head_battles() {
    PROVINCE_INDEX dislodger_into_weaker {-1};
    PROVINCE_INDEX dislodger_into_stronger {-1};
    UNIT_AND_ORDER *stronger_unit {nullptr};
    UNIT_AND_ORDER *weaker_unit {nullptr};

    // For each unbalanced head to head battle
    for (int unbalanced_head_to_head : unbalanced_head_to_heads) {

        // Get the two unit records
        stronger_unit = &(units[unbalanced_head_to_head]);
        weaker_unit = &(units[stronger_unit->move_dest.province_index]);

        // Check if the stronger unit dislodges the weaker unit
        dislodger_into_weaker = find_dislodging_unit(weaker_unit->coast_id.province_index, true);
        if (dislodger_into_weaker == stronger_unit->coast_id.province_index) {
            // It does, so the weaker unit is bounced and dislodged, and the stronger unit advances
            bounce_attack(weaker_unit);
            advance_unit(stronger_unit->coast_id.province_index);
            weaker_unit->dislodged = true;
            weaker_unit->dislodged_from = stronger_unit->coast_id.province_index;

        // Check if the stronger unit is dislodged
        } else {
            dislodger_into_stronger = find_dislodging_unit(stronger_unit->coast_id.province_index, true);

            // Bounce the weaker unit back
            bounce_attack(weaker_unit);

            // Advance the winning unit into the weaker units province
            if (dislodger_into_weaker != NO_DISLODGING_UNIT) {
                advance_unit(dislodger_into_weaker);
                weaker_unit->dislodged = true;
                weaker_unit->dislodged_from = dislodger_into_weaker;

            // Bounce all attacks on the weaker unit
            } else {
                bounce_all_attacks_on_province(weaker_unit->coast_id.province_index);
            }

            // If the stronger unit is also dislodged then dislodge it and advance the successful attacker
            if ((dislodger_into_stronger != NO_DISLODGING_UNIT)
                && (dislodger_into_stronger != weaker_unit->coast_id.province_index)) {

                advance_unit(dislodger_into_stronger);
                stronger_unit->dislodged = true;
                stronger_unit->dislodged_from = dislodger_into_stronger;

            // Bounce all attacks on the stronger unit
            } else {
                bounce_all_attacks_on_province(stronger_unit->coast_id.province_index);
            }
        }
    }
}

void MapAndUnits::resolve_balanced_head_to_head_battles() {
    PROVINCE_INDEX dislodger_into_second {-1};
    PROVINCE_INDEX dislodger_into_first {-1};
    UNIT_AND_ORDER *first_unit {nullptr};
    UNIT_AND_ORDER *second_unit {nullptr};

    // For each balanced head to head battle
    for (int balanced_head_to_head : balanced_head_to_heads) {

        // Get the two unit records
        first_unit = &(units[balanced_head_to_head]);
        second_unit = &(units[first_unit->move_dest.province_index]);

        // Find what dislodges each of them
        dislodger_into_first = find_dislodging_unit(first_unit->coast_id.province_index, true);
        dislodger_into_second = find_dislodging_unit(second_unit->coast_id.province_index, true);

        // Process the first.
        if ((dislodger_into_first == second_unit->coast_id.province_index)
            || (dislodger_into_first == NO_DISLODGING_UNIT)) {

            // All attacks bounce, or the strongest one is the head to head unit, so bounce all
            bounce_all_attacks_on_province(first_unit->coast_id.province_index);

        // Advance the strongest unit
        } else {
            advance_unit(dislodger_into_first);
            first_unit->dislodged = true;
            first_unit->dislodged_from = dislodger_into_first;
        }

        // Now process the second
        if ((dislodger_into_second == first_unit->coast_id.province_index)
            || (dislodger_into_second == NO_DISLODGING_UNIT)) {

            // All attacks bounce, or the strongest one is the head to head unit, so bounce all
            bounce_all_attacks_on_province(second_unit->coast_id.province_index);

        // Advance the strongest unit
        } else {
            advance_unit(dislodger_into_second);
            second_unit->dislodged = true;
            second_unit->dislodged_from = dislodger_into_second;
        }
    }
}

void MapAndUnits::fight_ordinary_battles() {
    // Just run through the attacker map, resolving each entry (each entry is removed
    // from the map once resolved, so we just keep resolving the first entry until
    // there are none left)
    while (!attacker_map.empty()) {
        auto attacker_itr = attacker_map.begin();
        resolve_attacks_on_province(attacker_itr->first);
    }
}

void MapAndUnits::resolve_attacks_on_province(PROVINCE_INDEX province) {
    PROVINCE_INDEX dislodging_unit {-1};
    UNIT_AND_ORDER *occupying_unit {nullptr};

    // Check if there is a unit already in the province
    auto occupying_unit_itr = units.find(province);
    if (occupying_unit_itr != units.end()) {

        // There is. If it is moving and hasn't been resolved yet, then resolve that unit first
        occupying_unit = &(occupying_unit_itr->second);

        if ((   (occupying_unit->order_type_copy == MOVE_ORDER)
                 || (occupying_unit->order_type_copy == MOVE_BY_CONVOY_ORDER)
            ) && !occupying_unit->unit_moves) {

            resolve_attacks_on_province(occupying_unit->move_dest.province_index);
        }

        // If it has moved, then the province is now empty
        if (occupying_unit->unit_moves) {
            occupying_unit_itr = units.end();
        }
    }

    // There is still a unit in the target province, so check if it is dislodged
    if (occupying_unit_itr != units.end()) {
        resolve_attacks_on_occupied_province(province);

    // Target province is empty, so check if anyone gets in
    } else {
        dislodging_unit = find_successful_attack_on_empty_province(province);

        // Nobody makes it. Bounce everyone
        if (dislodging_unit == NO_DISLODGING_UNIT) {
            bounce_all_attacks_on_province(province);

        // A unit does make it. Advance it.
        } else {
            advance_unit(dislodging_unit);
        }
    }
}

bool MapAndUnits::resolve_attacks_on_occupied_province(PROVINCE_INDEX attacked_province) {
    bool unit_dislodged {false};
    PROVINCE_INDEX dislodging_unit {-1};
    UNIT_AND_ORDER *occupying_unit {nullptr};

    // Find the unit that dislodges the occupier
    dislodging_unit = find_dislodging_unit(attacked_province);

    // Nobody successfully dislodges the occupier, so bounce everything
    if (dislodging_unit == NO_DISLODGING_UNIT) {
        bounce_all_attacks_on_province(attacked_province);
        unit_dislodged = false;

    } else {
        // If the occupier is supporting, then cut the support
        cut_support(attacked_province);

        // Advance the successful attacker
        advance_unit(dislodging_unit);

        // Dislodge the occupier
        occupying_unit = &(units[attacked_province]);
        occupying_unit->dislodged = true;
        occupying_unit->dislodged_from = dislodging_unit;
        unit_dislodged = true;
    }

    return unit_dislodged;
}

void MapAndUnits::cut_support(PROVINCE_INDEX attacked_province) {
    UNIT_AND_ORDER *cut_unit {nullptr};
    UNIT_AND_ORDER *supported_unit {nullptr};

    auto cut_unit_itr = units.find(attacked_province);
    if (cut_unit_itr != units.end()) {
        cut_unit = &(cut_unit_itr->second);

        // Subtract the support from the supported units total
        if ((cut_unit->order_type_copy == SUPPORT_TO_HOLD_ORDER)
            || (cut_unit->order_type_copy == SUPPORT_TO_MOVE_ORDER)) {
            supported_unit = &(units[cut_unit->other_source_province]);
        } else {
            supported_unit = nullptr;
        }

        if (supported_unit != nullptr) {
            supported_unit->supports.erase(cut_unit->coast_id.province_index);

            if (cut_unit->is_support_to_dislodge) {
                supported_unit->no_of_supports_to_dislodge--;
            }

            // Cut the support for the supporting unit
            cut_unit->order_type_copy = HOLD_ORDER;
            cut_unit->support_cut = true;
        }
    }
}

MapAndUnits::PROVINCE_INDEX
MapAndUnits::find_dislodging_unit(PROVINCE_INDEX attacked_province, bool ignore_occupying_unit) {
    int most_supports {-1};
    int most_supports_to_dislodge {-1};
    int second_most_supports {-1};
    PROVINCE_INDEX most_supported_unit {-1};
    UNIT_AND_ORDER *attacking_unit {nullptr};
    UNIT_AND_ORDER *occupying_unit {nullptr};

    // Find the strength of the most supported and second most supported unit
    for (auto attacking_unit_itr = attacker_map.lower_bound(attacked_province);
         attacking_unit_itr != attacker_map.upper_bound(attacked_province);
         attacking_unit_itr++) {

        attacking_unit = &(units[attacking_unit_itr->second]);

        if (attacking_unit->supports.size() > most_supports) {
            second_most_supports = most_supports;
            most_supports = attacking_unit->supports.size();
            most_supports_to_dislodge = attacking_unit->no_of_supports_to_dislodge;
            most_supported_unit = attacking_unit_itr->second;
        } else if (attacking_unit->supports.size() > second_most_supports) {
            second_most_supports = attacking_unit->supports.size();
        }
    }

    // If we have to consider the occupying unit, then consider it from second strongest
    if (!ignore_occupying_unit) {
        occupying_unit = &(units[attacked_province]);

        if (occupying_unit->supports.size() > second_most_supports) {
            second_most_supports = occupying_unit->supports.size();
        }
    }

    // Only if the most supported has more supports to dislodge than the second strongest has
    // to defend with, does any unit advance.
    if ((most_supports_to_dislodge <= second_most_supports) || (most_supports_to_dislodge <= 0)) {
        most_supported_unit = NO_DISLODGING_UNIT;
    }

    return most_supported_unit;
}

MapAndUnits::PROVINCE_INDEX MapAndUnits::find_successful_attack_on_empty_province(PROVINCE_INDEX attacked_province) {
    int most_supports {-1};
    int second_most_supports {-1};
    PROVINCE_INDEX most_supported_unit {-1};
    UNIT_AND_ORDER *attacking_unit {nullptr};

    // Find the strength of the most supported and second most supported unit
    for (auto attacking_unit_itr = attacker_map.lower_bound(attacked_province);
         attacking_unit_itr != attacker_map.upper_bound(attacked_province);
         attacking_unit_itr++) {

        attacking_unit = &(units[attacking_unit_itr->second]);

        if (attacking_unit->supports.size() > most_supports) {
            second_most_supports = most_supports;
            most_supports = attacking_unit->supports.size();
            most_supported_unit = attacking_unit_itr->second;
        } else if (attacking_unit->supports.size() > second_most_supports) {
            second_most_supports = attacking_unit->supports.size();
        }
    }

    // If the strongest is not more than the second strongest, then nothing advances
    if (most_supports <= second_most_supports) {
        most_supported_unit = NO_DISLODGING_UNIT;
    }

    return most_supported_unit;
}

void MapAndUnits::adjudicate_retreats() {
    ATTACKER_MAP retreat_map;
    UNIT_AND_ORDER *unit {nullptr};
    UNIT_AND_ORDER *bouncing_unit {nullptr};

    // Set up units to start adjudicating
    for (auto &dislodged_unit : dislodged_units) {
        unit = &(dislodged_unit.second);
        unit->order_type_copy = unit->order_type;
        unit->bounce = false;
        unit->unit_moves = false;
    }

    if (check_orders_on_adjudication) {
        check_for_illegal_retreat_orders();
    }

    // Check each unit in turn
    for (auto &dislodged_unit : dislodged_units) {
        unit = &(dislodged_unit.second);

        // It's retreating. Check if we know of another unit retreating to its space
        if (unit->order_type_copy == RETREAT_ORDER) {
            auto bouncing_unit_itr = retreat_map.find(unit->move_dest.province_index);

            // Found one, so bounce both of them
            if (bouncing_unit_itr != retreat_map.end()) {
                unit->bounce = true;
                bouncing_unit = &(dislodged_units[bouncing_unit_itr->second]);
                bouncing_unit->unit_moves = false;
                bouncing_unit->bounce = true;

            // No bounce found, so assume unit moves for now.
            } else {
                retreat_map.insert(ATTACKER_MAP::value_type(unit->move_dest.province_index,
                                                            unit->coast_id.province_index));
                unit->unit_moves = true;
            }

        // Nothing to do. Unit doesn't move, but isn't bounced.
        } else {}
    }
}

void MapAndUnits::check_for_illegal_retreat_orders() {
    UNIT_AND_ORDER *unit_record {nullptr};      // The record for the unit being ordered

    for (auto &dislodged_unit : dislodged_units) {
        unit_record = &(dislodged_unit.second);

        if (!can_move_to(unit_record, unit_record->move_dest)) {
            unit_record->order_type_copy = HOLD_ORDER;
            unit_record->illegal_order = true;
            unit_record->illegal_reason = TOKEN_ORDER_NOTE_FAR;
        } else if ((bounce_locations.find(unit_record->move_dest.province_index) != bounce_locations.end())
                   || (units.find(unit_record->move_dest.province_index) != units.end())
                   || (unit_record->dislodged_from == unit_record->move_dest.province_index)) {
            unit_record->order_type_copy = HOLD_ORDER;
            unit_record->illegal_order = true;
            unit_record->illegal_reason = TOKEN_ORDER_NOTE_NVR;
        }
    }
}

void MapAndUnits::adjudicate_builds() {
    WINTER_ORDERS_FOR_POWER *orders {nullptr};

    // For each power, check if they have ordered enough builds/disbands
    for (POWER_INDEX power_index = 0; power_index < number_of_powers; power_index++) {
        orders = &(winter_orders[power_index]);

        if (orders->is_building) {
            if (orders->builds_or_disbands.size() + orders->number_of_waives < orders->number_of_orders_required) {
                orders->number_of_waives = orders->number_of_orders_required - orders->builds_or_disbands.size();
            }
        } else {
            if (orders->builds_or_disbands.size() < orders->number_of_orders_required) {
                generate_cd_disbands(power_index, orders);
            }
        }
    }

    // The builds are all valid. Just mark them as such.
    for (POWER_INDEX power_index = 0; power_index < number_of_powers; power_index++) {
        orders = &(winter_orders[power_index]);

        for (auto &builds_or_disband : orders->builds_or_disbands) {
            builds_or_disband.second = TOKEN_RESULT_SUC;
        }
    }
}

void MapAndUnits::generate_cd_disbands(POWER_INDEX power_index, WINTER_ORDERS_FOR_POWER *orders) {
    using DISTANCE_FROM_HOME_MAP = std::multimap<int, PROVINCE_INDEX>;
    DISTANCE_FROM_HOME_MAP distance_from_home_map;

    for (auto &unit : units) {
        if (unit.second.nationality == power_index) {
            distance_from_home_map.insert(
                    DISTANCE_FROM_HOME_MAP::value_type(get_distance_from_home(unit.second), unit.first));
        }
    }

    for (auto distance_itr = distance_from_home_map.rbegin();
         distance_itr != distance_from_home_map.rend();
         distance_itr++) {

        if ((orders->builds_or_disbands.size() < orders->number_of_orders_required)
             && (orders->builds_or_disbands.find(units[distance_itr->second].coast_id)
                 == orders->builds_or_disbands.end())) {

            orders->builds_or_disbands.insert(
                    BUILDS_OR_DISBANDS::value_type(units[distance_itr->second].coast_id, TOKEN_ORDER_NOTE_MBV));
        }
    }
}

int MapAndUnits::get_distance_from_home(UNIT_AND_ORDER &unit) {
    using DISTANCE_MAP = std::map<PROVINCE_INDEX, int>;

    bool home_centre_found {false};
    int best_current_distance {0};
    DISTANCE_MAP distance_map;
    DISTANCE_MAP current_distances;
    DISTANCE_MAP new_distances;
    HOME_CENTRE_SET *home_centre_set {nullptr};
    PROVINCE_COASTS *coast_details {nullptr};

    home_centre_set = &(game_map[unit.coast_id.province_index].home_centre_set);

    // If the unit is not already in a home centre
    if (home_centre_set->find(unit.nationality) == home_centre_set->end()) {

        // Put this one province in the current distances
        current_distances.insert(DISTANCE_MAP::value_type(unit.coast_id.province_index, 0));

        // While a home centre is not found
        while (!home_centre_found) {
            best_current_distance++;

            for (auto distance_itr = current_distances.begin();
                 distance_itr != current_distances.end();
                 distance_itr++) {

                coast_details = &(game_map[distance_itr->first].coast_info);

                for (auto &coast_detail : *coast_details) {

                    for (const auto &adjacent_coast : coast_detail.second.adjacent_coasts) {

                        if ((distance_map.find(adjacent_coast.province_index) == distance_map.end())
                            && (current_distances.find(adjacent_coast.province_index) == current_distances.end())) {

                            new_distances.insert(
                                    DISTANCE_MAP::value_type(adjacent_coast.province_index, best_current_distance));
                            home_centre_set = &(game_map[adjacent_coast.province_index].home_centre_set);

                            if (home_centre_set->find(unit.nationality) != home_centre_set->end()) {
                                home_centre_found = true;
                            }
                        }
                    }
                }
            }

            for (auto &current_distance : current_distances) {
                distance_map.insert(current_distance);
            }

            current_distances = new_distances;
            new_distances.clear();
        }
    }

    return best_current_distance;
}

bool MapAndUnits::apply_adjudication() {
    if ((current_season == TOKEN_SEASON_SPR) || (current_season == TOKEN_SEASON_FAL)) {
        apply_moves();
    } else if ((current_season == TOKEN_SEASON_SUM) || (current_season == TOKEN_SEASON_AUT)) {
        apply_retreats();
    } else {
        apply_builds();
    }
    return move_to_next_turn();
}

void MapAndUnits::apply_moves() {
    UNITS moved_units {};
    UNIT_AND_ORDER *unit {nullptr};
    COAST_SET *adjacency_list {nullptr};

    // Move all the moved units aside. Move all the dislodged units into the dislodged units map
    dislodged_units.clear();

    auto unit_itr = units.begin();
    while (unit_itr != units.end()) {
        unit = &(unit_itr->second);

        // Clear the units order
        unit->order_type = NO_ORDER;

        if (unit->unit_moves) {
            moved_units.insert(UNITS::value_type(unit->move_dest.province_index, *unit));
            unit_itr = units.erase(unit_itr);

        } else if (unit->dislodged) {
            dislodged_units.insert(UNITS::value_type(unit->coast_id.province_index, *unit));
            unit_itr = units.erase(unit_itr);

        } else {
            unit_itr++;
        }
    }

    // Move the moved units back into the main map in their new place.
    for (auto &moved_unit : moved_units) {
        unit = &(moved_unit.second);
        unit->coast_id = unit->move_dest;
        units.insert(UNITS::value_type(unit->move_dest.province_index, *unit));
    }

    // For each dislodged unit, set its retreat options
    for (auto &dislodged_unit : dislodged_units) {
        dislodged_unit.second.retreat_options.clear();
        adjacency_list = &(game_map[dislodged_unit.second.coast_id.province_index]
                           .coast_info[dislodged_unit.second.coast_id.coast_token]
                           .adjacent_coasts);

        for (const auto &adjacency_itr : *adjacency_list) {
            if ((adjacency_itr.province_index != dislodged_unit.second.dislodged_from)
                 && (units.find(adjacency_itr.province_index) == units.end())
                 && (bounce_locations.find(adjacency_itr.province_index) == bounce_locations.end())) {

                dislodged_unit.second.retreat_options.insert(adjacency_itr);
            }
        }
    }
}

void MapAndUnits::apply_retreats() {
    UNIT_AND_ORDER *unit {nullptr};

    // Move all the moved units aside. Move all the dislodged units into the dislodged units map
    for (auto &dislodged_unit : dislodged_units) {
        unit = &(dislodged_unit.second);

        // Clear the units order
        unit->order_type = NO_ORDER;

        if (unit->unit_moves) {
            unit->coast_id = unit->move_dest;
            units.insert(UNITS::value_type(unit->move_dest.province_index, *unit));
        }
    }
    dislodged_units.clear();
}

void MapAndUnits::apply_builds() {
    UNIT_AND_ORDER new_unit;
    WINTER_ORDERS_FOR_POWER *orders {nullptr};

    for (POWER_INDEX power_ctr = 0; power_ctr < number_of_powers; power_ctr++) {
        orders = &(winter_orders[power_ctr]);

        // Add a new unit to the unit map
        if (orders->is_building) {
            for (auto &builds_or_disband : orders->builds_or_disbands) {
                new_unit.coast_id = builds_or_disband.first;
                new_unit.nationality = power_ctr;
                new_unit.order_type = NO_ORDER;

                if (new_unit.coast_id.coast_token == TOKEN_UNIT_AMY) {
                    new_unit.unit_type = TOKEN_UNIT_AMY;
                } else {
                    new_unit.unit_type = TOKEN_UNIT_FLT;
                }

                units.insert(UNITS::value_type(new_unit.coast_id.province_index, new_unit));
            }

        // Remove a unit from the unit map
        } else {
            for (auto &builds_or_disband : orders->builds_or_disbands) {
                units.erase(builds_or_disband.first.province_index);
            }
        }
    }
}

bool MapAndUnits::move_to_next_turn() {
    bool new_turn_found {false};
    bool send_sco {false};              // Whether SCO should be sent if server.

    // Step through the turns until we find one which has something to do
    while (!new_turn_found) {
        if (current_season == TOKEN_SEASON_WIN) {
            current_season = TOKEN_SEASON_SPR;
            current_year++;
        } else {
            current_season = Token(current_season.get_token() + 1);
        }

        // Movement turns always happen
        if ((current_season == TOKEN_SEASON_SPR) || (current_season == TOKEN_SEASON_FAL)) {
            new_turn_found = true;

        // Retreat turns happen if there are any dislodged units
        } else if ((current_season == TOKEN_SEASON_SUM) || (current_season == TOKEN_SEASON_AUT)) {
            if (!dislodged_units.empty()) {
                new_turn_found = true;
            }

        // Maybe a winter turn, so update SC ownership and see if anyone has adjustments to
        } else {
            if (update_sc_ownership()) {
                new_turn_found = true;
            }

            // Send an SCO message whether we have a winter or not
            send_sco = true;
        }
    }

    return send_sco;
}

bool MapAndUnits::update_sc_ownership() {
    bool orders_required {false};
    int unit_count[MAX_POWERS] {0};
    int sc_count[MAX_POWERS] {0};
    UNIT_AND_ORDER *unit {nullptr};
    WINTER_ORDERS_FOR_POWER *orders {nullptr};

    for (int power_ctr = 0; power_ctr < number_of_powers; power_ctr++) {
        unit_count[power_ctr] = 0;
        sc_count[power_ctr] = 0;
    }

    // Update the ownership of all occupied provinces, and count units
    for (auto &unit_itr : units) {
        unit = &(unit_itr.second);
        game_map[unit->coast_id.province_index].owner = Token(CATEGORY_POWER, unit->nationality);
        unit_count[unit->nationality]++;
    }

    // Count SCs
    for (PROVINCE_INDEX province_index = 0; province_index < number_of_provinces; province_index++) {
        if ((game_map[province_index].is_supply_centre) && (game_map[province_index].owner != TOKEN_PARAMETER_UNO)) {
            sc_count[game_map[province_index].owner.get_subtoken()]++;
        }
    }

    // Work out who is building and who is disbanding
    for (int power_ctr = 0; power_ctr < number_of_powers; power_ctr++) {
        orders = &(winter_orders[power_ctr]);

        if (sc_count[power_ctr] > unit_count[power_ctr]) {
            orders->is_building = true;
            orders->number_of_orders_required = sc_count[power_ctr] - unit_count[power_ctr];
        } else {
            orders->is_building = false;
            orders->number_of_orders_required = unit_count[power_ctr] - sc_count[power_ctr];
        }

        if (sc_count[power_ctr] != unit_count[power_ctr]) {
            orders_required = true;
        }

        orders->number_of_waives = 0;
        orders->builds_or_disbands.clear();
    }

    return orders_required;
}
