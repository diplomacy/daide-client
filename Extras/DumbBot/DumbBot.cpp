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

#include "StdAfx.h"
#include "JPN_String.h"
#include "DumbBot.h"
#include "Bot_Type.h"
#include "token_text_map.h"
#include "error_log.h"

const int NEUTRAL_POWER_INDEX = MapAndUnits::MAX_POWERS;

/**
 * How it works.
 *
 * DumbBot works in two stages. In the first stage it calculates a value for
 * each province and each coast. Then in the second stage, it works out an
 * order for each unit, based on those values.
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
 *                                * proximity[ n-1 ] for this coast )
 *                              / 5
 *
 * Also for each province, it works out the strength of attack we have on that
 * province - the number of adjacent units we have, and the competition for
 * that province - the number of adjacent units any one other power has.
 *
 * Finally it works out an overall value for each coast, based on all the proximity
 * values for that coast, and the strength and competition for that province, each
 * of which is multiplied by a weighting
 *
 * Then for move orders, it tries to move to the best adjacent coast, with a random
 * chance of it moving to the second best, third best, etc (with that chance varying
 * depending how close to first the second place is, etc). If the best place is where
 * it already is, it holds. If the best place already has an occupying unit, or
 * already has a unit moving there, it either supports that unit, or moves elsewhere,
 * depending on whether the other unit is guaranteed to succeed or not.
 *
 * Retreats are the same, except there are no supports...
 *
 * Builds and Disbands are also much the same - build in the highest value home centre
 * disband the unit in the lowest value location.
 **/

DumbBot::DumbBot( void )
{
	// Initialise all the settings. These are just values that seemed
	// right at the time - there has been no attempt to optimise them
	// for best play.

	// Importance of attacking centres we don't own, in Spring
	m_proximity_spring_attack_weight = 700;

	// Importance of defending our own centres in Spring
	m_proximity_spring_defence_weight = 300;

	// Same for fall.
	m_proximity_fall_attack_weight = 600;
	m_proximity_fall_defence_weight = 400;

	// Importance of proximity[n] in Spring
	m_spring_proximity_weight[ 0 ] = 100;
	m_spring_proximity_weight[ 1 ] = 1000;
	m_spring_proximity_weight[ 2 ] = 30;
	m_spring_proximity_weight[ 3 ] = 10;
	m_spring_proximity_weight[ 4 ] = 6;
	m_spring_proximity_weight[ 5 ] = 5;
	m_spring_proximity_weight[ 6 ] = 4;
	m_spring_proximity_weight[ 7 ] = 3;
	m_spring_proximity_weight[ 8 ] = 2;
	m_spring_proximity_weight[ 9 ] = 1;

	// Importance of our attack strength on a province in Spring
	m_spring_strength_weight = 1000;

	// Importance of lack of competition for the province in Spring
	m_spring_competition_weight = 1000;

	// Importance of proximity[n] in Fall
	m_fall_proximity_weight[ 0 ] = 1000;
	m_fall_proximity_weight[ 1 ] = 100;
	m_fall_proximity_weight[ 2 ] = 30;
	m_fall_proximity_weight[ 3 ] = 10;
	m_fall_proximity_weight[ 4 ] = 6;
	m_fall_proximity_weight[ 5 ] = 5;
	m_fall_proximity_weight[ 6 ] = 4;
	m_fall_proximity_weight[ 7 ] = 3;
	m_fall_proximity_weight[ 8 ] = 2;
	m_fall_proximity_weight[ 9 ] = 1;

	// Importance of our attack strength on a province in Fall
	m_fall_strength_weight = 1000;

	// Importance of lack of competition for the province in Fall
	m_fall_competition_weight = 1000;

	// Importance of building in provinces we need to defend
	m_build_defence_weight = 1000;

	// Importance of proximity[n] when building
	m_build_proximity_weight[ 0 ] = 1000;
	m_build_proximity_weight[ 1 ] = 100;
	m_build_proximity_weight[ 2 ] = 30;
	m_build_proximity_weight[ 3 ] = 10;
	m_build_proximity_weight[ 4 ] = 6;
	m_build_proximity_weight[ 5 ] = 5;
	m_build_proximity_weight[ 6 ] = 4;
	m_build_proximity_weight[ 7 ] = 3;
	m_build_proximity_weight[ 8 ] = 2;
	m_build_proximity_weight[ 9 ] = 1;

	// Importance of removing in provinces we don't need to defend
	m_remove_defence_weight = 1000;

	// Importance of proximity[n] when removing
	m_remove_proximity_weight[ 0 ] = 1000;
	m_remove_proximity_weight[ 1 ] = 100;
	m_remove_proximity_weight[ 2 ] = 30;
	m_remove_proximity_weight[ 3 ] = 10;
	m_remove_proximity_weight[ 4 ] = 6;
	m_remove_proximity_weight[ 5 ] = 5;
	m_remove_proximity_weight[ 6 ] = 4;
	m_remove_proximity_weight[ 7 ] = 3;
	m_remove_proximity_weight[ 8 ] = 2;
	m_remove_proximity_weight[ 9 ] = 1;

	// Percentage change of automatically playing the best move
	m_play_alternative = 50;

	// If not automatic, chance of playing best move if inferior move is nearly as good
	m_alternative_difference_modifier = 500;

	// Formula for power size. These are A,B,C in S = Ax^2 + Bx + C where C is centre count
	// and S is size of power
	m_size_square_coefficient = 1;
	m_size_coefficient = 4;
	m_size_constant = 16;
}

DumbBot::~DumbBot()
{
}

void DumbBot::send_nme_or_obs( void )
{
	send_name_and_version_to_server(BOT_FAMILY, BOT_GENERATION);
}

void DumbBot::process_mdf_message( TokenMessage &incoming_message )
{
	int province_counter;
	MapAndUnits::PROVINCE_COASTS *province_coasts;
	MapAndUnits::PROVINCE_COASTS::iterator coast_iterator;
	MapAndUnits::COAST_SET *adjacent_coasts;
	MapAndUnits::COAST_SET::iterator adjacent_coast_iterator;

	// Build the set of adjacent provinces
	for ( province_counter = 0; province_counter < m_map_and_units->number_of_provinces; province_counter++ )
	{
		province_coasts = &( m_map_and_units->game_map[ province_counter ].coast_info );

		for ( coast_iterator = province_coasts->begin();
			  coast_iterator != province_coasts->end();
			  coast_iterator++ )
		{
			adjacent_coasts = &( coast_iterator->second.adjacent_coasts );

			for ( adjacent_coast_iterator = adjacent_coasts->begin();
				  adjacent_coast_iterator != adjacent_coasts->end();
				  adjacent_coast_iterator++ )
			{
				m_adjacent_provinces[ province_counter ].insert( adjacent_coast_iterator->province_index );
			}
		}
	}
}

void DumbBot::process_sco_message( TokenMessage &incoming_message )
{
	int province_counter;
	int power_counter;

	for ( power_counter = 0; power_counter < MapAndUnits::MAX_POWERS + 1; power_counter++ )
	{
		m_power_size[ power_counter ] = 0;
	}

	// Count the size of each power
	for ( province_counter = 0; province_counter < m_map_and_units->number_of_provinces; province_counter++ )
	{
		if ( m_map_and_units->game_map[ province_counter ].is_supply_centre )
		{
			m_power_size[ get_power_index( m_map_and_units->game_map[ province_counter ].owner ) ]++;
		}
	}

	// Modify it using the Ax^2 + Bx + C formula
	for ( power_counter = 0; power_counter < MapAndUnits::MAX_POWERS + 1; power_counter++ )
	{
		m_power_size[ power_counter ]
			= m_size_square_coefficient * m_power_size[ power_counter ] * m_power_size[ power_counter ]
			+ m_size_coefficient * m_power_size[ power_counter ]
			+ m_size_constant;
	}
}

// Calculate the defense value. The defence value of a centre
// is the size of the largest enemy who has a unit which can
// move into the province

DumbBot::WEIGHTING DumbBot::calculate_defence_value( int province_index )
{
	WEIGHTING defence_value = 0;
	ADJACENT_PROVINCE_SET::iterator adjacent_province_iterator;
	MapAndUnits::UNITS::iterator adjacent_unit_iterator;
	MapAndUnits::COAST_SET *adjacent_unit_adjacent_coasts;
	MapAndUnits::COAST_ID coast_id;

	// For each adjacent province
	for ( adjacent_province_iterator = m_adjacent_provinces[ province_index ].begin();
		  adjacent_province_iterator != m_adjacent_provinces[ province_index ].end();
		  adjacent_province_iterator++ )
	{
		// If there is a unit there
		adjacent_unit_iterator = m_map_and_units->units.find( *adjacent_province_iterator );

		if ( adjacent_unit_iterator != m_map_and_units->units.end() )
		{
			// If it would increase the defence value
			if ( ( m_power_size[ adjacent_unit_iterator->second.nationality ] > defence_value )
			  && ( adjacent_unit_iterator->second.nationality != m_map_and_units->power_played.get_subtoken() ) )
			{
				// If it can move to this province
				adjacent_unit_adjacent_coasts = &( m_map_and_units->game_map[ *adjacent_province_iterator ].coast_info[ adjacent_unit_iterator->second.coast_id.coast_token ].adjacent_coasts );

				coast_id.province_index = province_index;
				coast_id.coast_token = Token( 0 );

				if ( ( adjacent_unit_adjacent_coasts->lower_bound( coast_id ) != adjacent_unit_adjacent_coasts->end() )
				  && ( adjacent_unit_adjacent_coasts->lower_bound( coast_id )->province_index == province_index ) )
				{
					// Update the defence value
					defence_value = m_power_size[ adjacent_unit_iterator->second.nationality ];
				}
			}
		}
	}

	return defence_value;
}

int DumbBot::get_power_index( Token power_token )
{
	int power_index;

	if ( power_token.get_category() == CATEGORY_POWER )
	{
		power_index = power_token.get_subtoken();
	}
	else
	{
		power_index = NEUTRAL_POWER_INDEX;
	}

	return power_index;
}

void DumbBot::process_now_message( TokenMessage &incoming_message )
{
	MapAndUnits::UNIT_SET::iterator unit_iterator;

	if ( ( m_map_and_units->current_season == TOKEN_SEASON_SPR )
	  || ( m_map_and_units->current_season == TOKEN_SEASON_SUM ) )
	{
		// Spring Moves/Retreats
		calculate_factors( m_proximity_spring_attack_weight, m_proximity_spring_defence_weight );

		calculate_destination_value( m_spring_proximity_weight, m_spring_strength_weight, m_spring_competition_weight );
	}
	else if ( ( m_map_and_units->current_season == TOKEN_SEASON_FAL )
		   || ( m_map_and_units->current_season == TOKEN_SEASON_AUT ) )
	{
		// Fall Moves/Retreats
		calculate_factors( m_proximity_fall_attack_weight, m_proximity_fall_defence_weight );

		calculate_destination_value( m_fall_proximity_weight, m_fall_strength_weight, m_fall_competition_weight );
	}
	else
	{
		// Adjustments
		calculate_factors( m_proximity_spring_attack_weight, m_proximity_spring_defence_weight );

		if ( m_map_and_units->our_units.size() > m_map_and_units->our_centres.size() )
		{
			// Disbanding
			calculate_winter_destination_value( m_remove_proximity_weight, m_remove_defence_weight );
		}
		else
		{
			// Building
			calculate_winter_destination_value( m_build_proximity_weight, m_build_defence_weight );
		}
	}

	if (dump_ai) {
		generate_debug();
	}

	if ( ( m_map_and_units->current_season == TOKEN_SEASON_SPR )
	  || ( m_map_and_units->current_season == TOKEN_SEASON_FAL ) )
	{
		generate_movement_orders();
	}
	else if ( ( m_map_and_units->current_season == TOKEN_SEASON_SUM )
		   || ( m_map_and_units->current_season == TOKEN_SEASON_AUT ) )
	{
		generate_retreat_orders();
	}
	else
	{
		if ( m_map_and_units->our_units.size() > m_map_and_units->our_centres.size() )
		{
			// Too many units. Remove.
			generate_remove_orders( m_map_and_units->our_units.size() - m_map_and_units->our_centres.size() );
		}
		else if ( m_map_and_units->our_units.size() < m_map_and_units->our_centres.size() )
		{
			// Not enough units. Builds.
			generate_build_orders( m_map_and_units->our_centres.size() - m_map_and_units->our_units.size() );
		}
	}

	send_orders_to_server();
}

void DumbBot::calculate_factors( WEIGHTING proximity_attack_weight, WEIGHTING proximity_defence_weight )
{
	int province_counter;
	MapAndUnits::PROVINCE_COASTS *province_coasts;
	MapAndUnits::PROVINCE_COASTS::iterator province_coast_iterator;
	MapAndUnits::COAST_ID coast_id;
	int previous_province;
	WEIGHTING previous_weight;
	int proximity_counter;
	MapAndUnits::COAST_SET *adjacent_coasts;
	MapAndUnits::COAST_SET::iterator coast_iterator;
	PROXIMITY_MAP::iterator proximity_iterator;
	int adjacent_unit_count[ MapAndUnits::MAX_PROVINCES ][ MapAndUnits::MAX_POWERS ];
	MapAndUnits::UNITS::iterator unit_iterator;
	int power_counter;

	// Initialise arrays to 0
	for ( province_counter = 0; province_counter < m_map_and_units->number_of_provinces; province_counter++ )
	{
		m_attack_value[ province_counter ] = 0;
		m_defence_value[ province_counter ] = 0;
	}

	// Calculate attack and defense values for each province
	for ( province_counter = 0; province_counter < m_map_and_units->number_of_provinces; province_counter++ )
	{
		if ( m_map_and_units->game_map[ province_counter ].is_supply_centre )
		{
			if ( m_map_and_units->game_map[ province_counter ].owner == m_map_and_units->power_played )
			{
				// Our SC. Calc defense value
				m_defence_value[ province_counter ] = calculate_defence_value( province_counter );
			}
			else
			{
				// Not ours. Calc attack value (which is the size of the owning power)
				m_attack_value[ province_counter ] = m_power_size[ get_power_index( m_map_and_units->game_map[ province_counter ].owner ) ];
			}
		}
	}

	// Calculate proximities[ 0 ].
	// Proximity[0] is calculated based on the attack value and defence value of the province,
	// modified by the weightings for the current season.
	for ( province_counter = 0; province_counter < m_map_and_units->number_of_provinces; province_counter++ )
	{
		province_coasts = &( m_map_and_units->game_map[ province_counter ].coast_info );
		coast_id.province_index = province_counter;

		for ( province_coast_iterator = province_coasts->begin();
			  province_coast_iterator != province_coasts->end();
			  province_coast_iterator++ )
		{
			coast_id.coast_token = province_coast_iterator->first;

			( m_proximity_map[ 0 ] )[ coast_id ] = m_attack_value[ province_counter ] * proximity_attack_weight
												 + m_defence_value[ province_counter ] * proximity_defence_weight;

			// Clear proximity[ 1... ]
			for ( proximity_counter = 1; proximity_counter < PROXIMITY_DEPTH; proximity_counter++ )
			{
				( m_proximity_map[ proximity_counter ] )[ coast_id ] = 0;
			}
		}
	}

	// Calculate proximities [ 1... ]
	// proximity[n] = ( sum of proximity[n-1] for this coast plus all coasts
	// which this coast is adjacent to ) / 5
	// The divide by 5 is just to keep all values of proximity[ n ] in the same range.
	// The average coast has 4 adjacent coasts, plus itself.
	for ( proximity_counter = 1; proximity_counter < PROXIMITY_DEPTH; proximity_counter++ )
	{
		for ( proximity_iterator = m_proximity_map[ proximity_counter ].begin();
			  proximity_iterator != m_proximity_map[ proximity_counter ].end();
			  proximity_iterator++ )
		{
			adjacent_coasts = &( m_map_and_units->game_map[ proximity_iterator->first.province_index ].coast_info[ proximity_iterator->first.coast_token ].adjacent_coasts );

			previous_province = -1;

			for ( coast_iterator = adjacent_coasts->begin();
				  coast_iterator != adjacent_coasts->end();
				  coast_iterator++ )
			{
				// If it is the same province again (e.g. This is Spa/sc after Spa/nc, for MAO
				if ( coast_iterator->province_index == previous_province )
				{
					// Replace the previous weight with the new one, if the new one is higher
					if ( ( m_proximity_map[ proximity_counter - 1 ] )[ *coast_iterator ] > previous_weight )
					{
						proximity_iterator->second = proximity_iterator->second - previous_weight;

						previous_weight = ( m_proximity_map[ proximity_counter - 1 ] )[ *coast_iterator ];

						proximity_iterator->second = proximity_iterator->second + previous_weight;
					}
				}
				else
				{
					// Add to the weight
					previous_weight = ( m_proximity_map[ proximity_counter - 1 ] )[ *coast_iterator ];

					proximity_iterator->second = proximity_iterator->second + previous_weight;
				}

				previous_province = coast_iterator->province_index;
			}

			// Add this province in, then divide the answer by 5
			proximity_iterator->second = proximity_iterator->second + ( m_proximity_map[ proximity_counter - 1 ] )[ proximity_iterator->first ];

			proximity_iterator->second = proximity_iterator->second / 5;
		}
	}

	// Calculate number of units each power has next to each province
	for ( province_counter = 0; province_counter < m_map_and_units->number_of_provinces; province_counter++ )
	{
		for ( power_counter = 0; power_counter < m_map_and_units->number_of_powers; power_counter++ )
		{
			adjacent_unit_count[ province_counter][ power_counter ] = 0;
		}

		// Strength value is the number of units we have next to the province
		m_strength_value[ province_counter ] = 0;

		// Competition value is the greatest number of units any other power has next to the province
		m_competition_value[ province_counter ] = 0;
	}

	for ( unit_iterator = m_map_and_units->units.begin();
		  unit_iterator != m_map_and_units->units.end();
		  unit_iterator++ )
	{
		adjacent_coasts = &( m_map_and_units->game_map[ unit_iterator->second.coast_id.province_index ].coast_info[ unit_iterator->second.coast_id.coast_token ].adjacent_coasts );
		
		for ( coast_iterator = adjacent_coasts->begin();
			  coast_iterator != adjacent_coasts->end();
			  coast_iterator++ )
		{
			adjacent_unit_count[ coast_iterator->province_index ][ unit_iterator->second.nationality ]++;
		}

		adjacent_unit_count[ unit_iterator->second.coast_id.province_index ][ unit_iterator->second.nationality ]++;
	}

	for ( province_counter = 0; province_counter < m_map_and_units->number_of_provinces; province_counter++ )
	{
		for ( power_counter = 0; power_counter < m_map_and_units->number_of_powers; power_counter++ )
		{
			if ( power_counter == m_map_and_units->power_played.get_subtoken() )
			{
				m_strength_value[ province_counter ] = adjacent_unit_count[ province_counter ][ power_counter ];
			}
			else if ( adjacent_unit_count[ province_counter ][ power_counter ] > m_competition_value[ province_counter ] )
			{
				m_competition_value[ province_counter ] = adjacent_unit_count[ province_counter ][ power_counter ];
			}
		}
	}
}

// Given the province and coast calculated values, and the weighting for this turn, calculate
// the value of each coast.

void DumbBot::calculate_destination_value( WEIGHTING *proximity_weight, WEIGHTING strength_weight, WEIGHTING competition_weight )
{
	PROXIMITY_MAP::iterator proximity_iterator;
	MapAndUnits::COAST_ID coast_id;
	int proximity_counter;
	WEIGHTING destination_weight;

	for ( proximity_iterator = m_proximity_map[ 0 ].begin();
		  proximity_iterator != m_proximity_map[ 0 ].end();
		  proximity_iterator++ )
	{
		coast_id = proximity_iterator->first;

		destination_weight = 0;

		for ( proximity_counter = 0; proximity_counter < PROXIMITY_DEPTH; proximity_counter++ )
		{
			destination_weight += ( m_proximity_map[ proximity_counter ] )[ coast_id ] * proximity_weight[ proximity_counter ];
		}

		destination_weight += strength_weight * m_strength_value[ coast_id.province_index ];
		destination_weight -= competition_weight * m_competition_value[ coast_id.province_index ];

		m_destination_value[ coast_id ] = destination_weight;
	}
}

// Given the province and coast calculated values, and the weighting for this turn, calculate
// the value of each coast for winter builds and removals.

void DumbBot::calculate_winter_destination_value( WEIGHTING *proximity_weight, WEIGHTING defence_weight )
{
	PROXIMITY_MAP::iterator proximity_iterator;
	MapAndUnits::COAST_ID coast_id;
	int proximity_counter;
	WEIGHTING destination_weight;

	for ( proximity_iterator = m_proximity_map[ 0 ].begin();
		  proximity_iterator != m_proximity_map[ 0 ].end();
		  proximity_iterator++ )
	{
		coast_id = proximity_iterator->first;

		destination_weight = 0;

		for ( proximity_counter = 0; proximity_counter < PROXIMITY_DEPTH; proximity_counter++ )
		{
			destination_weight += ( m_proximity_map[ proximity_counter ] )[ coast_id ] * proximity_weight[ proximity_counter ];
		}

		destination_weight += defence_weight * m_defence_value[ coast_id.province_index ];

		m_destination_value[ coast_id ] = destination_weight;
	}
}

// Generate a .csv file (readable by Excel and most other spreadsheets) which shows the
// data used to calculate the moves for this turn. Lists all the calculated information
// for every province we have a unit in or adjacent to.

void DumbBot::generate_debug( void )
{
	FILE *fp;
	PROXIMITY_MAP::iterator coast_iterator;
	MapAndUnits::COAST_ID coast_id;
	String filename;
	int proximity_counter;

	filename.Format("%d_%.4d_%.1d.csv", m_map_and_units->power_played.get_subtoken(), 
				m_map_and_units->current_year, m_map_and_units->current_season.get_subtoken() );

	fp = fopen( (const char *)( filename ), "w" );

	if ( fp == NULL )
	{
		display("Couldn't open debug file " + filename );
	}
	else
	{
		fprintf( fp, "Province,Coast,Attack,Defence,Strength,Competition,Proximities,,,,,,,,,,Value\n");

		for ( coast_iterator = m_proximity_map[ 0 ].begin();
			  coast_iterator != m_proximity_map[ 0 ].end();
			  coast_iterator++ )
		{
			coast_id = coast_iterator->first;

			if ( m_strength_value[ coast_id.province_index ] > 0 )
			{
				fprintf( fp, "%s,%s,",
						TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ coast_id.province_index ].province_token ].c_str(),
						TokenTextMap::instance()->m_token_to_text_map[ coast_id.coast_token ].c_str() );

				fprintf( fp, "%d,%d,%d,%d",
						m_attack_value[ coast_id.province_index ],
						m_defence_value[ coast_id.province_index ],
						m_strength_value[ coast_id.province_index ],
						m_competition_value[ coast_id.province_index ] );

				for ( proximity_counter = 0; proximity_counter < PROXIMITY_DEPTH; proximity_counter++ )
				{
					fprintf( fp, ",%d", ( m_proximity_map[ proximity_counter ] )[ coast_id ] );
				}

				fprintf( fp, ",%d", m_destination_value[ coast_id ] );

				fprintf( fp, "\n");
			}
		}

		fclose( fp );
	}
}

// Generate the actual orders for a movement turn

void DumbBot::generate_movement_orders( void )
{
	RANDOM_UNIT_MAP::iterator unit_iterator;
	DESTINATION_MAP destination_map;
	MapAndUnits::UNIT_AND_ORDER *unit;
	MapAndUnits::COAST_SET *adjacent_coasts;
	MapAndUnits::COAST_SET::iterator coast_iterator;
	DESTINATION_MAP::iterator destination_iterator;
	DESTINATION_MAP::iterator next_destination_iterator;
	bool try_next_province;
	WEIGHTING next_province_chance;
	bool selection_is_ok;
	bool order_unit_to_move;
	MapAndUnits::UNITS::iterator current_unit_iterator;
	RANDOM_UNIT_MAP::iterator random_unit_iterator;
	int occupying_unit_order;
	MOVING_UNIT_MAP moving_unit_map;
	MOVING_UNIT_MAP::iterator moving_unit_iterator;

	// Put our units into a random order. This is one of the ways in which
	// DumbBot is made non-deterministic - the order the units are considered
	// in can affect the orders selected
	generate_random_unit_list( m_map_and_units->our_units );

	unit_iterator = m_random_unit_map.begin();

	log("%s %d",
			TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->current_season ].c_str(),
			m_map_and_units->current_year );

	while ( unit_iterator != m_random_unit_map.end() )
	{
		// Build the list of possible destinations for this unit, indexed by the 
		// weight for the destination
		destination_map.clear();

		unit = &( m_map_and_units->units[ unit_iterator->second ] );

		log("Selecting destination for %s",
						TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ unit->coast_id.province_index ].province_token ].c_str() );


		// Put all the adjacent coasts into the destination map
		adjacent_coasts = &( m_map_and_units->game_map[ unit_iterator->second ].coast_info[ unit->coast_id.coast_token ].adjacent_coasts );

		for ( coast_iterator = adjacent_coasts->begin();
			  coast_iterator != adjacent_coasts->end();
			  coast_iterator++ )
		{
			destination_map.insert( DESTINATION_MAP::value_type( m_destination_value[ *coast_iterator ], *coast_iterator ) );
		}

		// Put the current location in (we can hold rather than move)
		destination_map.insert( DESTINATION_MAP::value_type( m_destination_value[ unit->coast_id ] + 1, unit->coast_id ) );

		do
		{
			// Pick a destination
			destination_iterator = destination_map.begin();

			log("Destination selected : %s %s",
						TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ destination_iterator->second.province_index ].province_token ].c_str(),
						TokenTextMap::instance()->m_token_to_text_map[ destination_iterator->second.coast_token ].c_str() );

			try_next_province = true;

			while ( try_next_province == true )
			{
				// Consider the next destination in the map
				next_destination_iterator = destination_iterator;
				next_destination_iterator++;

				// If there is no next destination, give up.
				if ( next_destination_iterator == destination_map.end() )
				{
					try_next_province = false;
				}
				else
				{
					// Randomly determine whether to use the current or next destination,
					// based on their relative weights (the higher the difference, the
					// more chance of picking the better one).
					if ( destination_iterator->first == 0 )
					{
						next_province_chance = 0;
					}
					else
					{
						next_province_chance = ( ( destination_iterator->first - next_destination_iterator->first ) * m_alternative_difference_modifier )
											 / destination_iterator->first;
					}

					if ( ( rand_no( 100 ) < m_play_alternative )
					  && ( rand_no( 100 ) >= next_province_chance ) )
					{
						// Pick the next one (and go around the loop again to keep considering 
						// more options
						destination_iterator = next_destination_iterator;

						log("New destination selected : %s %s (%d%% chance)",
									TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ destination_iterator->second.province_index ].province_token ].c_str(),
									TokenTextMap::instance()->m_token_to_text_map[ destination_iterator->second.coast_token ].c_str(),
									m_play_alternative - ( next_province_chance * m_play_alternative / 100 ) );
					}
					else
					{
						// Use this one
						try_next_province = false;
					}
				}
			}

			// Check whether this is a reasonable selection
			selection_is_ok = true;
			order_unit_to_move = true;

			// If this is a hold order
			if ( destination_iterator->second.province_index == unit->coast_id.province_index )
			{
				log("Ordered to hold");

				// Hold order
				m_map_and_units->set_hold_order( unit_iterator->second );
			}
			else
			{
				// If there is a unit in this province already 
				current_unit_iterator = m_map_and_units->units.find( destination_iterator->second.province_index );
				if ( m_map_and_units->our_units.find( destination_iterator->second.province_index ) != m_map_and_units->our_units.end() )
				{
					log("Province occupied");

					// If it is not yet ordered
					if ( current_unit_iterator->second.order_type == MapAndUnits::NO_ORDER )
					{
						log("Occupying unit unordered");

						// Find this unit in the random unit map
						for ( random_unit_iterator = m_random_unit_map.begin();
							  random_unit_iterator != m_random_unit_map.end();
							  random_unit_iterator++ )
						{
							if ( random_unit_iterator->second == destination_iterator->second.province_index )
							{
								occupying_unit_order = random_unit_iterator->first;
							}
						}

						// Move this unit to after the one it's waiting for. We can't decide
						// whether to move there or not, so give up on this unit for now
						if ( occupying_unit_order >= 0 )
						{
							m_random_unit_map.insert( RANDOM_UNIT_MAP::value_type( occupying_unit_order - 1, unit_iterator->second ) );

							order_unit_to_move = false;
						}
					}

					// If it is not moving
					else if ( ( current_unit_iterator->second.order_type != MapAndUnits::MOVE_ORDER )
						   && ( current_unit_iterator->second.order_type != MapAndUnits::MOVE_BY_CONVOY_ORDER ) )
					{
						log("Occupying unit not moving");

						// If it needs supporting
						if ( m_competition_value[ destination_iterator->second.province_index ] > 1 )
						{
							log("Supporting occupying unit");

							// Support it
							m_map_and_units->set_support_to_hold_order( unit_iterator->second, destination_iterator->second.province_index );

							order_unit_to_move = false;
						}
						else
						{
							log("Destination not accepted");

							// This is not an acceptable selection
							selection_is_ok = false;

							// Make sure it isn't selected again
							destination_map.erase( destination_iterator );
						}
					}
				}

				// If there is a unit already moving to this province
				moving_unit_iterator = moving_unit_map.find( destination_iterator->second.province_index );
				if ( moving_unit_iterator != moving_unit_map.end() )
				{
					log("Unit already moving here");

					// If it may need support
					if ( m_competition_value[ destination_iterator->second.province_index ] > 0 )
					{
						log("Supporting moving unit");

						// Support it
						m_map_and_units->set_support_to_move_order( unit_iterator->second, moving_unit_iterator->second, moving_unit_iterator->first );

						order_unit_to_move = false;
					}
					else
					{
						log("Destination not accepted");

						// This is not an acceptable selection
						selection_is_ok = false;

						// Make sure it isn't selected again
						destination_map.erase( destination_iterator );
					}
				}

				if ( ( selection_is_ok ) && ( order_unit_to_move ) )
				{
					log("Ordered to move");

					m_map_and_units->set_move_order( unit_iterator->second, destination_iterator->second );

					moving_unit_map[ destination_iterator->second.province_index ] = unit_iterator->second;
				}
			}
		}
		while ( selection_is_ok == false );

		// Unit is now ordered, so delete it from the random unit map
		m_random_unit_map.erase( unit_iterator );

		// Move onto the next one
		unit_iterator = m_random_unit_map.begin();
	}

	check_for_wasted_holds( moving_unit_map );
}

void DumbBot::check_for_wasted_holds( MOVING_UNIT_MAP &moving_unit_map )
{
	MapAndUnits::UNIT_SET::iterator our_unit_iterator;
	MapAndUnits::UNIT_AND_ORDER *unit;
	MapAndUnits::COAST_SET *adjacent_coasts;
	MapAndUnits::COAST_SET::iterator adjacent_coast_iterator;
	MapAndUnits::UNITS::iterator adjacent_unit_iterator;
	WEIGHTING max_destination_value;
	MapAndUnits::PROVINCE_INDEX destination;
	MapAndUnits::PROVINCE_INDEX source;

	// For each unit, if it is ordered to hold
	for ( our_unit_iterator = m_map_and_units->our_units.begin();
		  our_unit_iterator != m_map_and_units->our_units.end();
		  our_unit_iterator++ )
	{
		unit = &( m_map_and_units->units[ *our_unit_iterator ] );

		if ( unit->order_type == MapAndUnits::HOLD_ORDER )
		{
			// Consider every province we can move to
			adjacent_coasts = &( m_map_and_units->game_map[ unit->coast_id.province_index ].coast_info[ unit->coast_id.coast_token ].adjacent_coasts );

			max_destination_value = 0;

			for ( adjacent_coast_iterator = adjacent_coasts->begin();
				  adjacent_coast_iterator != adjacent_coasts->end();
				  adjacent_coast_iterator++ )
			{
				// Check if there is a unit moving there
				if ( moving_unit_map.find( adjacent_coast_iterator->province_index ) != moving_unit_map.end() )
				{
					// Unit is moving there
					if ( m_competition_value[ adjacent_coast_iterator->province_index ] > 0 )
					{
						// Unit needs support
						if ( m_destination_value[ *adjacent_coast_iterator ] > max_destination_value )
						{
							// Best so far
							max_destination_value = m_destination_value[ *adjacent_coast_iterator ];
							destination = adjacent_coast_iterator->province_index;
							source = moving_unit_map[ adjacent_coast_iterator->province_index ];
						}
					}
				}
				else
				{
					// Check if there is a unit holding there
					adjacent_unit_iterator = m_map_and_units->units.find( adjacent_coast_iterator->province_index );

					if ( ( adjacent_unit_iterator != m_map_and_units->units.end() )
					  && ( adjacent_unit_iterator->second.nationality == m_map_and_units->power_played.get_subtoken() )
					  && ( adjacent_unit_iterator->second.order_type != MapAndUnits::MOVE_ORDER )
					  && ( adjacent_unit_iterator->second.order_type != MapAndUnits::MOVE_BY_CONVOY_ORDER ) )
					{
						// Unit is holding there
						if ( m_competition_value[ adjacent_coast_iterator->province_index ] > 1 )
						{
							// Unit needs support
							if ( m_destination_value[ *adjacent_coast_iterator ] > max_destination_value )
							{
								// Best so far
								max_destination_value = m_destination_value[ *adjacent_coast_iterator ];
								destination = adjacent_coast_iterator->province_index;
								source = destination;
							}
						}
					}
				}
			}

			// If something worth supporting was found
			if ( max_destination_value > 0 )
			{
				// Support it
				log("Overriding hold order in %s with support from %s to %s",
						TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ unit->coast_id.province_index ].province_token ].c_str(),
						TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ source ].province_token ].c_str(),
						TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ destination ].province_token ].c_str() );

				if ( source == destination )
				{
					m_map_and_units->set_support_to_hold_order( unit->coast_id.province_index, destination );
				}
				else
				{
					m_map_and_units->set_support_to_move_order( unit->coast_id.province_index, source, destination );
				}
			}
		}
	}
}

// Generate Retreat orders

void DumbBot::generate_retreat_orders( void )
{
	RANDOM_UNIT_MAP::iterator unit_iterator;
	DESTINATION_MAP destination_map;
	MapAndUnits::UNIT_AND_ORDER *unit;
	MapAndUnits::COAST_SET::iterator coast_iterator;
	DESTINATION_MAP::iterator destination_iterator;
	DESTINATION_MAP::iterator next_destination_iterator;
	bool try_next_province;
	WEIGHTING next_province_chance;
	bool selection_is_ok;
	MOVING_UNIT_MAP moving_unit_map;
	MOVING_UNIT_MAP::iterator moving_unit_iterator;

	// Put the units into a list in random order
	generate_random_unit_list( m_map_and_units->our_dislodged_units );

	unit_iterator = m_random_unit_map.begin();

	log("%s %d",
			TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->current_season ].c_str(),
			m_map_and_units->current_year );

	while ( unit_iterator != m_random_unit_map.end() )
	{
		destination_map.clear();

		unit = &( m_map_and_units->dislodged_units[ unit_iterator->second ] );

		log("Selecting destination for %s",
						TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ unit->coast_id.province_index ].province_token ].c_str() );

		// Put all the adjacent coasts into the destination map
		for ( coast_iterator = unit->retreat_options.begin();
			  coast_iterator != unit->retreat_options.end();
			  coast_iterator++ )
		{
			destination_map.insert( DESTINATION_MAP::value_type( m_destination_value[ *coast_iterator ], *coast_iterator ) );
		}

		do
		{
			// Pick a destination
			destination_iterator = destination_map.begin();

			if ( destination_iterator == destination_map.end() )
			{
				// No retreat possible. Disband unit
				m_map_and_units->set_disband_order( unit_iterator->second );

				selection_is_ok = true;

				log("Disbanding unit");
			}
			else
			{
				log("Destination selected : %s %s",
							TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ destination_iterator->second.province_index ].province_token ].c_str(),
							TokenTextMap::instance()->m_token_to_text_map[ destination_iterator->second.coast_token ].c_str() );

				// Determine whether to try the next option instead
				try_next_province = true;

				while ( try_next_province == true )
				{
					next_destination_iterator = destination_iterator;
					next_destination_iterator++;

					// If there is no next option, don't try it...
					if ( next_destination_iterator == destination_map.end() )
					{
						try_next_province = false;
					}
					else
					{
						// Randomly decide whether to move on
						if ( destination_iterator->first == 0 )
						{
							next_province_chance = 0;
						}
						else
						{
							next_province_chance = ( ( destination_iterator->first - next_destination_iterator->first ) * m_alternative_difference_modifier )
												 / destination_iterator->first;
						}

						if ( ( rand_no( 100 ) < m_play_alternative )
						  && ( rand_no( 100 ) >= next_province_chance ) )
						{
							// Move on
							destination_iterator = next_destination_iterator;

							log("New destination selected : %s %s (%d%% chance)",
										TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ destination_iterator->second.province_index ].province_token ].c_str(),
										TokenTextMap::instance()->m_token_to_text_map[ destination_iterator->second.coast_token ].c_str(),
										m_play_alternative - ( next_province_chance * m_play_alternative / 100 ) );
						}
						else
						{
							// Stick with this province
							try_next_province = false;
						}
					}
				}

				// Check whether this is a reasonable selection
				selection_is_ok = true;

				// If there is a unit already moving to this province
				moving_unit_iterator = moving_unit_map.find( destination_iterator->second.province_index );
				if ( moving_unit_iterator != moving_unit_map.end() )
				{
					log("Unit already moving here");

					// This is not an acceptable selection
					selection_is_ok = false;

					// Make sure it isn't selected again
					destination_map.erase( destination_iterator );
				}

				if ( selection_is_ok )
				{
					log("Ordered to RETREAT");

					// Order the retreat
					m_map_and_units->set_retreat_order( unit_iterator->second, destination_iterator->second );

					moving_unit_map[ destination_iterator->second.province_index ] = unit_iterator->second;
				}
			}
		}
		while ( selection_is_ok == false );

		// Unit is now ordered, so delete it from the random unit map
		m_random_unit_map.erase( unit_iterator );

		// Move onto the next one
		unit_iterator = m_random_unit_map.begin();
	}
}

// Generate the actual remove orders for an adjustment phase

void DumbBot::generate_remove_orders( int remove_count )
{
	MapAndUnits::UNIT_SET::iterator unit_iterator;
	DESTINATION_MAP remove_map;
	MapAndUnits::UNIT_AND_ORDER *unit;
	int remove_counter;
	DESTINATION_MAP::reverse_iterator remove_iterator;
	DESTINATION_MAP::iterator remove_forward_iterator;
	DESTINATION_MAP::reverse_iterator next_remove_iterator;
	bool try_next_remove;
	WEIGHTING next_remove_chance;

	// Put all the units into a removal map, ordered by value of their location
	for ( unit_iterator = m_map_and_units->our_units.begin();
		  unit_iterator != m_map_and_units->our_units.end();
		  unit_iterator++ )
	{
		unit = &( m_map_and_units->units[ *unit_iterator ] );

		remove_map[ m_destination_value[ unit->coast_id ] ] = unit->coast_id;
	}

	// For each required removal
	for ( remove_counter = 0; remove_counter < remove_count; remove_counter++ )
	{
		// Start with the best option (which is the end of the list)
		remove_iterator = remove_map.rbegin();

		log("Removal selected : %s %s",
					TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ remove_iterator->second.province_index ].province_token ].c_str(),
					TokenTextMap::instance()->m_token_to_text_map[ remove_iterator->second.coast_token ].c_str() );

		try_next_remove = true;

		// Determine whether to try the next option
		while ( try_next_remove )
		{
			next_remove_iterator = remove_iterator;
			next_remove_iterator++;

			// No next option - so don't try it.
			if ( next_remove_iterator == remove_map.rend() )
			{
				try_next_remove = false;
			}
			else
			{
				// Decide randomly
				if ( remove_iterator->first == 0 )
				{
					next_remove_chance = 0;
				}
				else
				{
					next_remove_chance = ( ( remove_iterator->first - next_remove_iterator->first ) * m_alternative_difference_modifier )
										/ remove_iterator->first;
				}

				if ( ( rand_no( 100 ) < m_play_alternative )
				  && ( rand_no( 100 ) >= next_remove_chance ) )
				{
					// Try the next one
					remove_iterator = next_remove_iterator;

					log("New removal selected : %s %s (%d%% chance)",
								TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ remove_iterator->second.province_index ].province_token ].c_str(),
								TokenTextMap::instance()->m_token_to_text_map[ remove_iterator->second.coast_token ].c_str(),
								m_play_alternative - ( next_remove_chance * m_play_alternative / 100 ) );
				}
				else
				{
					// Stick with this one
					try_next_remove = false;
				}
			}
		}

		// Order the removal
		m_map_and_units->set_remove_order( remove_iterator->second.province_index );

		remove_forward_iterator = remove_iterator.base();
		remove_forward_iterator--;
		remove_map.erase( remove_forward_iterator );
	}
}

// Generate the actual build orders for an adjustment turn

void DumbBot::generate_build_orders( int build_count )
{
	DESTINATION_MAP build_map;
	MapAndUnits::PROVINCE_SET::iterator home_centre_iterator;
	MapAndUnits::PROVINCE_COASTS *province_coasts;
	MapAndUnits::PROVINCE_COASTS::iterator build_coast_iterator;
	MapAndUnits::COAST_ID coast_id;
	int builds_remaining = build_count;
	DESTINATION_MAP::iterator build_iterator;
	DESTINATION_MAP::iterator next_build_iterator;
	bool try_next_build;
	WEIGHTING next_build_chance;
	int build_province;

	log("%s %d",
			TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->current_season ].c_str(),
			m_map_and_units->current_year );

	// Put all the coasts of all the home centres into a map
	for ( home_centre_iterator = m_map_and_units->open_home_centres.begin();
		  home_centre_iterator != m_map_and_units->open_home_centres.end();
		  home_centre_iterator++ )
	{
		province_coasts = &( m_map_and_units->game_map[ *home_centre_iterator ].coast_info );

		for ( build_coast_iterator = province_coasts->begin();
			  build_coast_iterator != province_coasts->end();
			  build_coast_iterator++ )
		{
			coast_id.province_index = *home_centre_iterator;
			coast_id.coast_token = build_coast_iterator->first;

			build_map.insert( DESTINATION_MAP::value_type( m_destination_value[ coast_id ], coast_id ) );
		}
	}

	// For each build, while we have a vacant home centre
	while ( ( build_map.empty() == false ) && ( builds_remaining > 0 ) )
	{
		// Select the best location
		build_iterator = build_map.begin();

		log("Build selected : %s %s",
					TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ build_iterator->second.province_index ].province_token ].c_str(),
					TokenTextMap::instance()->m_token_to_text_map[ build_iterator->second.coast_token ].c_str() );

		try_next_build = true;

		// Consider the next location
		while ( try_next_build )
		{
			next_build_iterator = build_iterator;
			next_build_iterator++;

			if ( next_build_iterator == build_map.end() )
			{
				// There isn't one, so stick with this one
				try_next_build = false;
			}
			else
			{
				// Determine randomly
				if ( build_iterator->first == 0 )
				{
					next_build_chance = 0;
				}
				else
				{
					next_build_chance = ( ( build_iterator->first - next_build_iterator->first ) * m_alternative_difference_modifier )
									  / build_iterator->first;
				}

				if ( ( rand_no( 100 ) < m_play_alternative )
				  && ( rand_no( 100 ) >= next_build_chance ) )
				{
					// Try the next one
					build_iterator = next_build_iterator;

					log("New build selected : %s %s (%d%% chance)",
								TokenTextMap::instance()->m_token_to_text_map[ m_map_and_units->game_map[ build_iterator->second.province_index ].province_token ].c_str(),
								TokenTextMap::instance()->m_token_to_text_map[ build_iterator->second.coast_token ].c_str(),
								m_play_alternative - ( next_build_chance * m_play_alternative / 100 ) );
				}
				else
				{
					// Stick with this one
					try_next_build = false;
				}
			}
		}

		// Order the build
		m_map_and_units->set_build_order( build_iterator->second );

		build_province = build_iterator->second.province_index;

		build_iterator = build_map.begin();

		// Remove all other build options for this province
		while ( build_iterator != build_map.end() )
		{
			if ( build_iterator->second.province_index == build_province )
			{
				build_iterator = build_map.erase( build_iterator );
			}
			else
			{
				build_iterator++;
			}
		}

		builds_remaining--;
	}

	// If not all builds ordered, order some waives.
	if ( builds_remaining > 0 )
	{
		log("Waiving %d builds", builds_remaining );

		m_map_and_units->set_total_number_of_waive_orders( builds_remaining );
	}
}

// Generate a random unit map from a set of units

void DumbBot::generate_random_unit_list( MapAndUnits::UNIT_SET &units )
{
	MapAndUnits::UNIT_SET::iterator unit_iterator;
	int unit_index;

	m_random_unit_map.clear();

	for ( unit_iterator = units.begin();
		  unit_iterator != units.end();
		  unit_iterator++ )
	{
		unit_index = rand_no( RAND_MAX );

		m_random_unit_map.insert( RANDOM_UNIT_MAP::value_type( unit_index, *unit_iterator ) );
	}
}

// Generate a random number

int DumbBot::rand_no( int max_value )
{
	int answer;

	answer = rand() % max_value;

	return answer;
}

bool DumbBot::extract_parameters( COMMAND_LINE_PARAMETERS &parameters )
{
	int search_start;					// Position to start searching command line
	int param_start;					// Start of the next parameter
	int param_end;						// End of the next parameter
	char param_token;					// The token specifying the parameter type
	String parameter;					// The parameter
	bool extracted_ok = true;			// Whether the parameters were OK
	
	parameters.ip_specified = false;
	parameters.name_specified = false;
	parameters.port_specified = false;
	parameters.log_level_specified = false;
	parameters.reconnection_specified = false;

	dump_ai = false;

	String m_command_line = GetCommandLineA();

	// Strip the program name off the command line
	if ( m_command_line[ 0 ] == '"' )
	{
		// Program name is in quotes.
		param_start = m_command_line.Find( '"', 1 );

		if ( param_start != -1 )
		{
			m_command_line = m_command_line.Mid( param_start + 1 );
		}
	}
	else
	{
		// Program name is not quoted, so is terminated by a space
		param_start = m_command_line.Find( ' ' );

		if ( param_start != -1 )
		{
			m_command_line = m_command_line.Mid( param_start );
		}
	}

	param_start = m_command_line.Find( '-', 0 );

	while ( param_start != -1 )
	{
		param_token = m_command_line[ param_start + 1 ];

		param_end = m_command_line.Find( ' ', param_start );

		if ( param_end == -1 )
		{
			parameter = m_command_line.Mid( param_start + 2 );

			search_start = m_command_line.GetLength();
		}
		else
		{
			parameter = m_command_line.Left( param_end ).Mid( param_start + 2 );

			search_start = param_end;
		}

		switch ( param_token )
		{
			case 's':
			{
				parameters.name_specified = true;
				parameters.server_name = parameter;
				
				break;
			}

			case 'i':
			{
				parameters.ip_specified = true;
				parameters.server_name = parameter;

				break;
			}

			case 'p':
			{
				parameters.port_specified = true;
				parameters.port_number = atoi( parameter );
				
				break;
			}

			case 'l':
			{
				parameters.log_level_specified = true;
				parameters.log_level = atoi( parameter );
				
				break;
			}

			case 'r':
			{
				if ( parameter[3] == ':' )
				{
					parameters.reconnection_specified = true;
					parameters.reconnect_power = parameter.Left( 3 );
					parameters.reconnect_power.MakeUpper();
					parameters.reconnect_passcode = atoi( parameter.Mid( 4 ) );
				}
				else
				{
					display("-r should be followed by 'POW:passcode'\n"
								  "POW should be three characters");
				}

				break;
			}

			case 'd':
			{
				dump_ai = true;
				break;
			}

			default:
			{
				display("Usage: "+String(BOT_FAMILY)+".exe [-sServerName|-iIPAddress] [-pPortNumber] "
					"[-lLogLevel] [-rPOW:passcode] [-d]");

				extracted_ok = false;
			}
		}

		param_start = m_command_line.Find('-', search_start );
	}

	if ( ( parameters.ip_specified ) && ( parameters.name_specified ) )
	{
		display("You must not specify Server name and IP address");

		extracted_ok = false;
	}

	return extracted_ok;
}

