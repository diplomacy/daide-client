/**
 * Diplomacy AI Client & Server - Part of the DAIDE project.
 *
 * Map And Unit Class. Handles all the map information and unit locations and orders
 *
 * (C) David Norman 2002 david@ellought.demon.co.uk
 *
 * This software may be reused for non-commercial purposes without charge, and
 * without notifying the author. Use of any part of this software for commercial 
 * purposes without permission from the Author is prohibited.
 *
 * Release 8~2
 **/

#ifndef _DAIDE_MAP_AND_UNITS
#define _DAIDE_MAP_AND_UNITS

#include "types.h"
#include "tokens.h"
#include "token_message.h"

/**
 * The MapAndUnits Class is a singleton. In order to access the class, the get_instance function
 * should be used. You cannot construct MapAndUnit objects outside the class.
 *
 * The MapAndUnits Class performs a number of functions.
 *
 * In a server, it is where the current position is stored, along with the details of the map.
 * The orders are also stored here as they are received from the clients, and then when required,
 * it performs the adjudication, and provides the results of the adjudication for sending back
 * to the clients.
 *
 * In a client, it is where the current position is stored, along with the details of the map.
 * The orders to be submitted are also stored here, and then they can be retrieved in a
 * TokenMessage which is suitable for sending to the server. Once the server has adjudicated,
 * the results are passed back to this class, which then updates the current position.
 *
 * The client can also get a copy of the current position in a new object by using the 
 * get_duplicate_instance function. They can then enter orders for all powers, and adjudicate
 * in order to find out what the resulting position would be. This could also be done with the
 * main instance, but this is not recommended, as it would leave the client with no record of
 * the current position.
 *
 * For every call to get_duplicate_instance(), you need to call delete_duplicate_instance() in
 * order to avoid memory leaks.
 **/

class MapAndUnits {
public:
    // Constants
    enum {
        MAX_PROVINCES = 256
    };
    enum {
        MAX_POWERS = 256
    };
    enum {
        NO_MAP = -1
    };            // Value used to indicate no map has been set up.

    // Types used by the MapAndUnits class

    // A province index. To convert from Token to PROVINCE_INDEX, use token.get_subtoken().
    // To convert from PROVINCE_INDEX to token, use game_map[ province_index ].province_token.
    typedef int PROVINCE_INDEX;

    // A power index. To convert from Token to POWER_INDEX, use token.get_subtoken().
    // To convert from POWER_INDEX to token, Token::Token( CATEGORY_POWER, power_index );
    typedef int POWER_INDEX;

    // A coast. The coast_token should be a TOKEN_COAST_... , or TOKEN_UNIT_FLT
    // for a province which does not have multiple coasts, or TOKEN_UNIT_AMY for
    // an army in the province.
    // Examples: Paris has one coast (TOKEN_UNIT_AMY). Brest has two coasts (TOKEN_UNIT_AMY and
    // TOKEN_UNIT_FLT). Spain has three coasts (TOKEN_UNIT_AMY, TOKEN_COAST_NCS and
    // TOKEN_COAST_SCS). North Sea has one coast (TOKEN_UNIT_FLT).
    typedef struct tag_coast_id {
        PROVINCE_INDEX province_index;
        Token coast_token;

        bool operator<(const struct tag_coast_id &other_coast_id) const {
            return ((province_index < other_coast_id.province_index)
                    || ((province_index == other_coast_id.province_index)
                        && (coast_token < other_coast_id.coast_token)));
        }
    } COAST_ID;

    // Orders which a unit can be given
    typedef enum {
        NO_ORDER,
        HOLD_ORDER,
        MOVE_ORDER,
        SUPPORT_TO_HOLD_ORDER,
        SUPPORT_TO_MOVE_ORDER,
        CONVOY_ORDER,
        MOVE_BY_CONVOY_ORDER,

        RETREAT_ORDER,
        DISBAND_ORDER,

        // The following can not be ordered, but may be used by the adjudicator
                HOLD_NO_SUPPORT_ORDER
    } ORDER_TYPE;

    // Used by the adjudicator
    typedef enum {
        RING_ADVANCES_REGARDLESS,
        RING_ADVANCES_IF_VACANT,
        STANDOFF_REGARDLESS,
        SIDE_ADVANCES_IF_VACANT,
        SIDE_ADVANCES_REGARDLESS
    } RING_UNIT_STATUS;

    // Collections
    typedef list<PROVINCE_INDEX> UNIT_LIST;
    typedef set<PROVINCE_INDEX> UNIT_SET;
    typedef set<PROVINCE_INDEX> PROVINCE_SET;
    typedef map<PROVINCE_INDEX, PROVINCE_INDEX> PROVINCE_TO_PROVINCE_MAP;
    typedef set<COAST_ID> COAST_SET;

    typedef struct {
        // Location of the unit
        COAST_ID coast_id;                // Coast of the unit's location
        POWER_INDEX nationality;        // Nationality of the unit
        Token unit_type;                // The type of unit

        // Order for the unit
        ORDER_TYPE order_type;            // Type of order given to unit
        COAST_ID move_dest;                // Destination to move to (normal or via convoy)
        PROVINCE_INDEX other_source_province;    // Province support or convoy is from (if moving)
        // (i.e. province containing supported unit)
        PROVINCE_INDEX other_dest_province;        // Province support or convoy is to (or is holding in)
        UNIT_LIST convoy_step_list;        // List of fleets that the convoy uses

        // The following are used during the adjudication
        ORDER_TYPE order_type_copy;        // Copy of the order type - may be reverted to HOLD or HOLD_NO_SUPPORT
        UNIT_SET supports;                // The supporting units
        int no_of_supports_to_dislodge;    // The number of supports which count towards dislodgement
        bool is_support_to_dislodge;    // Whether the support this unit is giving is a support to dislodge
        int move_number;                // Number of the move - used for detecting rings and head-to-heads
        RING_UNIT_STATUS ring_unit_status;        // Status of this unit as part of a ring of attack
        PROVINCE_INDEX dislodged_from;    // Province the unit was dislodged from

        // Results flags
        bool no_convoy;                    // Convoy order was not provided by necessary fleets
        bool no_army_to_convoy;            // The army was not ordered to use the convoy, or other fleets broke the route
        bool convoy_broken;                // A convoying fleet was dislodged so the convoy failed
        bool support_void;                // The support order did not match the order of the unit it was supporting
        bool support_cut;                // The support order was cut by an attack
        bool bounce;                    // The move bounced
        bool dislodged;                    // The unit was dislodged
        bool unit_moves;                // The unit moves successfully
        bool illegal_order;                // Order illegal - only possible in AOA game
        Token illegal_reason;            // Reason that order was illegal

        // Retreat options
        COAST_SET retreat_options;        // Locations where the unit can retreat to
    } UNIT_AND_ORDER;

    // The coasts adjacent to a given coast
    typedef struct {
        COAST_SET adjacent_coasts;
    } COAST_DETAILS;

    // The set of coasts in a province, keyed on the Coast Token
    // (may be TOKEN_COAST_..., TOKEN_UNIT_FLT or TOKEN_UNIT_AMY) - see COAST_ID above
    typedef map<Token, COAST_DETAILS> PROVINCE_COASTS;

    // The set of powers for which a given province is a home centre
    typedef set<POWER_INDEX> HOME_CENTRE_SET;

    // The details of a province
    typedef struct {
        Token province_token;
        bool province_in_use;
        bool is_supply_centre;
        bool is_land;
        PROVINCE_COASTS coast_info;
        HOME_CENTRE_SET home_centre_set;
        Token owner;
    } PROVINCE_DETAILS;

    // The collection of all units, keyed on the province in which the unit is located
    typedef map<PROVINCE_INDEX, UNIT_AND_ORDER> UNITS;

    // Map of build or disband orders against result token (SUC or failure reason)
    typedef map<COAST_ID, Token> BUILDS_OR_DISBANDS;

    // The winter orders for one power
    typedef struct {
        BUILDS_OR_DISBANDS builds_or_disbands;
        int number_of_waives;
        int number_of_orders_required;
        bool is_building;
    } WINTER_ORDERS_FOR_POWER;

    typedef map<POWER_INDEX, WINTER_ORDERS_FOR_POWER> WINTER_ORDERS;

    // Public Data.

    // The map
    PROVINCE_DETAILS game_map[MAX_PROVINCES];
    // The map
    int number_of_provinces;            // Number of provinces on the map
    int number_of_powers;                // The number of powers in the variant
    string map_name;                    // The name of the map in use
    Token power_played;                    // The power we are playing
    int passcode;                        // Our passcode
    TokenMessage variant;                // The variant details
    PROVINCE_SET home_centres;            // The home centres for the power being played

    // Current status
    bool game_started;                    // Whether the game has started
    bool game_over;                        // Whether the game is over
    Token current_season;                // The current season of play
    int current_year;                    // The current year of play
    UNITS units;                        // The non-dislodged units
    UNITS dislodged_units;                // The dislodged units
    WINTER_ORDERS winter_orders;        // The winter orders
    WINTER_ORDERS_FOR_POWER our_winter_orders;
    // The winter orders for this power
    UNITS last_movement_results;        // The results from the last movement turn
    UNITS last_retreat_results;            // The results from the last retreat turn
    WINTER_ORDERS last_adjustment_results;
    // The results from the last adjustment turn
    Token last_movement_result_season;    // The season for the last movement results

    // Server options
    bool check_orders_on_submission;    // Check orders fully on submit (ie not AOA game)
    bool check_orders_on_adjudication;    // Check orders fully on adjudicate (ie AOA game)

    // The following sets are built after each turn. You are welcome to destroy their contents
    // in the process of calculating the orders to submit (e.g. delete each unit as an order is
    // decided upon for it).
    UNIT_SET our_units;                    // The units owned by the power being played
    UNIT_SET our_dislodged_units;        // The units owned by the power being played which must retreat
    PROVINCE_SET open_home_centres;        // Home centres which are available for building in
    PROVINCE_SET our_centres;            // The centres owned by the power being played
    int number_of_disbands;                // The number of disbands required (negative for builds)

    // Public Functions

    // Get the instance of the MapAndUnits class
    static MapAndUnits *get_instance();

    // Get a duplicate copy of the MapAndUnits class. Recommended for messing around with
    // without affecting the master copy
    static MapAndUnits *get_duplicate_instance();

    // Delete a duplicate
    static void delete_duplicate_instance(MapAndUnits *duplicate);

    // Set up the class
    int set_map(TokenMessage &mdf_message);

    void set_power_played(Token power);

    int set_ownership(TokenMessage &sco_message);

    int set_units(TokenMessage &now_message);

    int store_result(TokenMessage &ord_message);

    // Set the order for a unit
    bool set_hold_order(PROVINCE_INDEX unit);

    bool set_move_order(PROVINCE_INDEX unit, COAST_ID destination);

    bool set_support_to_hold_order(PROVINCE_INDEX unit, PROVINCE_INDEX supported_unit);

    bool set_support_to_move_order(PROVINCE_INDEX unit, PROVINCE_INDEX supported_unit, PROVINCE_INDEX destination);

    bool set_convoy_order(PROVINCE_INDEX unit, PROVINCE_INDEX convoyed_army, PROVINCE_INDEX destination);

    bool set_move_by_convoy_order(PROVINCE_INDEX unit, PROVINCE_INDEX destination, int number_of_steps,
                                  PROVINCE_INDEX step_list[]);

    bool set_move_by_single_step_convoy_order(PROVINCE_INDEX unit, PROVINCE_INDEX destination, PROVINCE_INDEX step);

    bool set_disband_order(PROVINCE_INDEX unit);

    bool set_retreat_order(PROVINCE_INDEX unit, COAST_ID destination);

    void set_build_order(COAST_ID location);

    bool set_remove_order(PROVINCE_INDEX unit);

    void set_waive_order();

    void set_multiple_waive_orders(int waives);

    void set_total_number_of_waive_orders(int waives);

    // Accept a complete set of orders for a power as a single TokenMessage
    int process_orders(TokenMessage &sub_message, POWER_INDEX power_index, Token *order_result);

    // Cancel adjustment orders
    bool cancel_build_order(PROVINCE_INDEX location);

    bool cancel_remove_order(PROVINCE_INDEX location);

    bool unorder_adjustment(TokenMessage &not_sub_message, int power_index);

    // Whether any units have had orders submitted
    bool any_orders_entered();

    // Convert all the set orders into a sub command to send to the server
    TokenMessage build_sub_command();

    // Clear all orders
    void clear_all_orders();

    // Describe a result as a string. Used if results are to be displayed to the user.
    string describe_movement_result(UNIT_AND_ORDER &unit);

    string describe_movement_order_string(UNIT_AND_ORDER &unit, UNITS &unit_set);

    string describe_retreat_result(UNIT_AND_ORDER &unit);

    string describe_retreat_order_string(UNIT_AND_ORDER &unit);

    string describe_adjustment_result(WINTER_ORDERS_FOR_POWER &orders, int order_index);

    int get_number_of_results(WINTER_ORDERS_FOR_POWER &orders);

    COAST_ID find_result_unit_initial_location(PROVINCE_INDEX province_index,
                                               bool &is_new_build, bool &retreated_to_province, bool &moved_to_province,
                                               bool &unit_found);

    string describe_unit_string(UNIT_AND_ORDER &unit);

    string describe_coast_string(COAST_ID &coast);

    string describe_province_string(PROVINCE_INDEX province_index);

    // Get the details of the variant (returns false if variant not set in HLO command)
    bool get_variant_setting(const Token &variant_option,    // Variant to check
                             Token *parameter = nullptr);    // OUTPUT: Parameter for variant (if one was provided)

    // Get the adjacency list for a coast
    COAST_SET *get_adjacent_coasts(COAST_ID &coast);

    // Get the adjacency list for a unit. Returns nullptr if no unit in the given province.
    COAST_SET *get_adjacent_coasts(PROVINCE_INDEX &unit_location);

    // Get the adjacency list for a dislodged unit. Returns nullptr if no dislodged unit in the given province.
    COAST_SET *get_dislodged_unit_adjacent_coasts(PROVINCE_INDEX &dislodged_unit_location);

    // Functions for the adjudicator

    // Set the order checking options
    void set_order_checking(bool check_on_submit, bool check_on_adjudicate);

    // Perform the adjudication
    void adjudicate();

    // Get the results as a set of ORD messages
    int get_adjudication_results(TokenMessage ord_messages[]);

    // Apply the adjudication. This moves all the units to their new positions.
    bool apply_adjudication();

    // Get the current game status
    void get_unit_positions(TokenMessage *now_message);

    void get_sc_ownerships(TokenMessage *sco_message);

    int get_centre_count(Token power);

    int get_unit_count(Token power);

    // Check if every unit has an order
    bool check_if_all_orders_received(POWER_INDEX power_index);

    // Get the part of a NOW message for a single unit
    TokenMessage describe_unit(UNIT_AND_ORDER *unit);

    TokenMessage describe_dislodged_unit(UNIT_AND_ORDER *unit);

private:
    // Private constructor - use the get_instance() function
    MapAndUnits();

    int process_power_list(TokenMessage &power_list);

    int process_provinces(TokenMessage &provinces);

    int process_supply_centres(TokenMessage &supply_centres);

    int process_supply_centres_for_power(TokenMessage &supply_centres);

    int process_non_supply_centres(TokenMessage &non_supply_centres);

    int process_adjacencies(TokenMessage &adjacencies);

    int process_province_adjacency(TokenMessage &province_adjacency);

    int process_adjacency_list(PROVINCE_DETAILS *province_details, TokenMessage &adjacency_list);

    int process_sco_for_power(TokenMessage &sco_for_power);

    // From the client side
    int process_now_unit(TokenMessage unit);

    COAST_ID get_coast_id(TokenMessage &coast, Token unit_type);

    TokenMessage describe_movement_order(UNIT_AND_ORDER *unit);

    TokenMessage describe_turn();

    TokenMessage describe_coast(COAST_ID coast);

    TokenMessage describe_retreat_order(UNIT_AND_ORDER *unit);

    COAST_ID get_coast_from_unit(TokenMessage &unit);

    void decode_order(UNIT_AND_ORDER &unit, TokenMessage order);

    void decode_result(UNIT_AND_ORDER &unit, TokenMessage result);

    // From the server side
    int process_now_unit(TokenMessage unit_message, PROVINCE_SET &no_bounce_list,
                         PROVINCE_TO_PROVINCE_MAP &bounce_reason_list);

    Token process_order(TokenMessage &order, POWER_INDEX power_index);

    UNIT_AND_ORDER *find_unit(TokenMessage &unit_to_find, UNITS &units_map);

    bool can_move_to(UNIT_AND_ORDER *unit, COAST_ID destination);

    bool can_move_to_province(UNIT_AND_ORDER *unit, int province_index);

    bool has_route_to_province(UNIT_AND_ORDER *unit, PROVINCE_INDEX province_index, PROVINCE_INDEX province_to_avoid);

    int get_movement_results(TokenMessage ord_messages[]);

    TokenMessage describe_movement_result(UNIT_AND_ORDER *unit);

    int get_retreat_results(TokenMessage ord_messages[]);

    TokenMessage describe_retreat_result(UNIT_AND_ORDER *unit);

    int get_adjustment_results(TokenMessage ord_messages[]);

    TokenMessage describe_build_result(POWER_INDEX power_counter,
                                       WINTER_ORDERS_FOR_POWER *orders,
                                       BUILDS_OR_DISBANDS::iterator order_itr);

    TokenMessage describe_waive(POWER_INDEX power_counter);

    // Functions used to adjudicate
    void adjudicate_moves();

    void adjudicate_retreats();

    void adjudicate_builds();

    void initialise_move_adjudication();

    void check_for_illegal_move_orders();

    void cancel_inconsistent_convoys();

    void cancel_inconsistent_supports();

    void direct_attacks_cut_support();

    void build_support_lists();

    void build_convoy_subversion_list();

    bool resolve_attacks_on_non_subverted_convoys();

    bool check_for_futile_convoys();

    bool check_for_indomitable_and_futile_convoys();

    void resolve_circles_of_subversion();

    bool resolve_attacks_on_occupied_province(PROVINCE_INDEX attacked_province);

    void identify_rings_of_attack_and_head_to_head_battles();

    void advance_rings_of_attack();

    RING_UNIT_STATUS determine_ring_status(PROVINCE_INDEX province, PROVINCE_INDEX ring_unit_source);

    void advance_unit(PROVINCE_INDEX unit_to_advance);

    void bounce_all_attacks_on_province(PROVINCE_INDEX province_index);

    void bounce_attack(UNIT_AND_ORDER *unit);

    void resolve_unbalanced_head_to_head_battles();

    void resolve_balanced_head_to_head_battles();

    void fight_ordinary_battles();

    void resolve_attacks_on_province(PROVINCE_INDEX province);

    void cut_support(PROVINCE_INDEX attacked_province);

    PROVINCE_INDEX find_dislodging_unit(PROVINCE_INDEX attacked_province, bool ignore_occupying_unit = false);

    PROVINCE_INDEX find_successful_attack_on_empty_province(PROVINCE_INDEX attacked_province);

    void check_for_illegal_retreat_orders();

    void generate_cd_disbands(POWER_INDEX power_index, WINTER_ORDERS_FOR_POWER *orders);

    int get_distance_from_home(UNIT_AND_ORDER &unit);

    void apply_moves();

    void apply_retreats();

    void apply_builds();

    bool move_to_next_turn();

    bool update_sc_ownership();

    // Constants used by the adjudicator
    enum {
        DOES_NOT_SUBVERT = -1
    };
    enum {
        NO_DISLODGING_UNIT = -1
    };
    enum {
        NO_MOVE_NUMBER = -1
    };

    // Types used by the adjudicator
    typedef enum {
        NOT_SUBVERTED_CONVOY,
        SUBVERTED_CONVOY,
        FUTILE_CONVOY,
        INDOMINTABLE_CONVOY,
        CONFUSED_CONVOY
    } SUBVERSION_TYPE;

    typedef struct tag_convoy_subversion {
        PROVINCE_INDEX subverted_convoy_army;    // The army in the convoy this convoy subverts
        int number_of_subversions;                // Number of convoys which subvert this convoy
        SUBVERSION_TYPE subversion_type;        // How this convoy is subverted
    } CONVOY_SUBVERSION;

    typedef map<PROVINCE_INDEX, CONVOY_SUBVERSION> CONVOY_SUBVERSION_MAP;

    typedef multimap<PROVINCE_INDEX, PROVINCE_INDEX> ATTACKER_MAP;

    // Data used to adjudicate
    ATTACKER_MAP attacker_map;
    UNIT_SET supporting_units;
    UNIT_SET convoying_units;
    UNIT_SET convoyed_units;
    CONVOY_SUBVERSION_MAP convoy_subversions;
    UNIT_SET rings_of_attack;
    UNIT_SET balanced_head_to_heads;
    UNIT_SET unbalanced_head_to_heads;
    PROVINCE_SET bounce_locations;    // The locations in which bounces have occured

};

#endif // _DAIDE_MAP_AND_UNITS
