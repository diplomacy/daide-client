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

#ifndef DAIDE_CLIENT_BOTS_DUMBBOT_DUMBBOT_H
#define DAIDE_CLIENT_BOTS_DUMBBOT_DUMBBOT_H

#include "basebot.h"
#include "daide_client/map_and_units.h"

namespace DAIDE {

class DumbBot : public DAIDE::BaseBot {
public:
    DumbBot();
    DumbBot(const DumbBot &other) = delete;                 // Copy constructor
    DumbBot(DumbBot &&rhs) = delete;                        // Move constructor
    DumbBot& operator=(const DumbBot &other) = delete;      // Copy Assignment
    DumbBot& operator=(DumbBot &&rhs) = delete;             // Move Assignment
    ~DumbBot() override = default;

    void send_nme_or_obs() override;

    void process_mdf_message(const DAIDE::TokenMessage &incoming_msg) override;

    void process_sco_message(const DAIDE::TokenMessage &incoming_msg) override;

    void process_now_message(const DAIDE::TokenMessage &incoming_msg) override;

private:
    using WEIGHTING = int64_t;
    using ADJACENT_PROVINCE_SET = std::set<DAIDE::MapAndUnits::PROVINCE_INDEX>;
    using PROXIMITY_MAP = std::map<DAIDE::MapAndUnits::COAST_ID, WEIGHTING>;
    using RANDOM_UNIT_MAP = std::multimap<int, DAIDE::MapAndUnits::PROVINCE_INDEX, std::greater<>>;
    using DESTINATION_MAP = std::map<WEIGHTING, DAIDE::MapAndUnits::COAST_ID, std::greater<>>;
    using MOVING_UNIT_MAP = std::map<DAIDE::MapAndUnits::PROVINCE_INDEX, DAIDE::MapAndUnits::PROVINCE_INDEX>;

    enum { PROXIMITY_DEPTH = 10 };

    static int get_power_index(const DAIDE::Token &power_token);

    WEIGHTING calculate_defense_value(int province_index);

    void calculate_factors(WEIGHTING proximity_attack_weight, WEIGHTING proximity_defense_weight);

    void calculate_dest_value(const WEIGHTING *proximity_weight,
                              WEIGHTING strength_weight,
                              WEIGHTING competition_weight);

    void calculate_winter_dest_value(const WEIGHTING *proximity_weight, WEIGHTING defense_weight);

    void generate_movement_orders();

    void check_for_wasted_holds(MOVING_UNIT_MAP &moving_unit_map);

    void generate_retreat_orders();

    void generate_remove_orders(int disband_count);

    void generate_build_orders(int build_count);

    void generate_random_unit_list(const DAIDE::MapAndUnits::UNIT_SET &units);

    static int rand_no(int max_value);

    virtual bool extract_parameters(const std::string &command_line_a, DAIDE::COMMAND_LINE_PARAMETERS &parameters);

private:

    // The attack value for each province.
    WEIGHTING m_attack_value[DAIDE::MapAndUnits::MAX_PROVINCES];

    // The defense value for each province
    WEIGHTING m_defense_value[DAIDE::MapAndUnits::MAX_PROVINCES];

    // The size of each power. This is a function of the power's SC count
    int m_power_size[DAIDE::MapAndUnits::MAX_POWERS + 1];

    // The set of provinces adjacent to a given province
    ADJACENT_PROVINCE_SET m_adjacent_provinces[DAIDE::MapAndUnits::MAX_PROVINCES];

    // The proximity map.
    PROXIMITY_MAP m_proximity_map[PROXIMITY_DEPTH];

    // The strength of attack we have on the province
    WEIGHTING m_strength_value[DAIDE::MapAndUnits::MAX_PROVINCES];

    // The strength of the competition for this province from enemy units
    WEIGHTING m_competition_value[DAIDE::MapAndUnits::MAX_PROVINCES];

    // The value of each province
    PROXIMITY_MAP m_destination_value;

    // All our units which need orders, in a random order
    RANDOM_UNIT_MAP m_random_unit_map;

    // The weightings for each consideration. These are effectively constants, set
    // in the constructor. They could be loaded from file to allow DumbBot to be
    // tweaked

    // Importance of attacking centres we don't own, in Spring
    const static WEIGHTING m_proximity_spring_attack_weight {700};

    // Importance of defending our own centres in Spring
    const static WEIGHTING m_proximity_spring_defense_weight {300};

    // Same for fall.
    const static WEIGHTING m_proximity_fall_attack_weight {600};
    const static WEIGHTING m_proximity_fall_defense_weight {400};

    // Importance of proximity[n] in Spring
    constexpr static WEIGHTING m_spring_proximity_weight[PROXIMITY_DEPTH] {100, 1000, 30, 10, 6, 5, 4, 3, 2, 1};

    // Importance of our attack strength on a province in Spring
    const static WEIGHTING m_spring_strength_weight {1000};

    // Importance of lack of competition for the province in Spring
    const static WEIGHTING m_spring_competition_weight {1000};

    // Importance of proximity[n] in Fall
    constexpr static WEIGHTING m_fall_proximity_weight[PROXIMITY_DEPTH] {1000, 100, 30, 10, 6, 5, 4, 3, 2, 1};

    // Importance of our attack strength on a province in Fall
    const static WEIGHTING m_fall_strength_weight {1000};

    // Importance of lack of competition for the province in Fall
    const static WEIGHTING m_fall_competition_weight {1000};

    // Importance of building in provinces we need to defend
    const static WEIGHTING m_build_defense_weight {1000};

    // Importance of proximity[n] when building
    constexpr static WEIGHTING m_build_proximity_weight[PROXIMITY_DEPTH] {1000, 100, 30, 10, 6, 5, 4, 3, 2, 1};

    // Importance of removing in provinces we don't need to defend
    const static WEIGHTING m_remove_defense_weight {1000};

    // Importance of proximity[n] when removing
    constexpr static WEIGHTING m_remove_proximity_weight[PROXIMITY_DEPTH] {1000, 100, 30, 10, 6, 5, 4, 3, 2, 1};

    // Percentage change of automatically playing the best move
    const static WEIGHTING m_play_alternative {50};

    // If not automatic, chance of playing best move if inferior move is nearly as good
    const static WEIGHTING m_alternative_difference_modifier {500};

    // Formula for power size. These are A,B,C in S = Ax^2 + Bx + C where C is centre count
    // and S is size of power
    const static WEIGHTING m_size_square_coefficient {-1};
    const static WEIGHTING m_size_coefficient {4};
    const static WEIGHTING m_size_constant {16};
};

} // namespace DAIDE

#endif // DAIDE_CLIENT_BOTS_DUMBBOT_DUMBBOT_H
