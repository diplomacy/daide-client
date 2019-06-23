/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * BaseBot header file. This is the base class for all Bots.
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

#ifndef _DAN_BASEBOT_H
#define _DAN_BASEBOT_H

using namespace JPN;

#include "ai_client_types.h"
#include "token_message.h"
#include "socket.h"

typedef JPN::MessageHeader DCSP_HST_MESSAGE;

// DAIDE message types.
#define DCSP_MSG_TYPE_IM 0
#define DCSP_MSG_TYPE_RM 1
#define DCSP_MSG_TYPE_DM 2
#define DCSP_MSG_TYPE_FM 3
#define DCSP_MSG_TYPE_EM 4

class MapAndUnits;            // The map and units class

// BaseBot : The base class for all Bots

class BaseBot {
public:
    Socket m_socket;

    BaseBot();

    virtual ~BaseBot();

    // Initialise the AI. May be overridden, but should call the base class version at the top
    // of the derived version if it is
    virtual bool initialise();

protected:
    // Useful utility functions

    // Send initial message to the server
    void send_initial_message_to_server();

    // Send a message to the server
    void send_message_to_server(TokenMessage &message);

    // Send the set orders to the server (see set_..._order() in MapAndUnits)
    void send_orders_to_server();

    // Send a name and version to the server
    void BaseBot::send_name_and_version_to_server(const char *name, const char *version);

    // Disconnect from the server
    void disconnect_from_server();

    // Request a copy of the map. Gets a MAP and MDF, but doesn't YES(MAP()) in response
    // Done automatically if joining a game via HLO or OBS, but not if via IAM (since you
    // may just be rejoining following connection loss).
    void request_map();

    // Determine if an MAP or MDF was by request.
    // Only valid in process_map_message and process_mdf_message
    bool is_map_requested();

    // Overrideables

    // The following virtual functions have a default implementation, but you may completely override them.

    // Send the NME or OBS message to server. Default sends OBS.
    virtual void send_nme_or_obs();

    // Handle an incoming CCD message.
    virtual void process_ccd_message(TokenMessage &incoming_message, bool is_new_disconnection);

    // Handle an incoming DRW message. Default sets the game to over
    virtual void process_drw_message(TokenMessage &incoming_message);

    // Handle an incoming FRM message. Default version replies with HUH( message ). TRY().
    virtual void process_frm_message(TokenMessage &incoming_message);

    // Handle an incoming HUH message. Default logs it
    virtual void process_huh_message(TokenMessage &incoming_message);

    // Handle an incoming LOD message. Default replies with REJ (LOD (...) )
    virtual void process_lod_message(TokenMessage &incoming_message);

    // Handle an incoming MIS message.
    virtual void process_mis_message(TokenMessage &incoming_message);

    // Handle an incoming OFF message. Default disconnects from server and exits
    virtual void process_off_message(TokenMessage &incoming_message);

    // Handle an incoming OUT message.
    virtual void process_out_message(TokenMessage &incoming_message);

    // Handle an incoming PRN message. Default logs it
    virtual void process_prn_message(Token *incoming_message, int message_length);

    // Handle an incoming SMR message.
    virtual void process_smr_message(TokenMessage &incoming_message);

    // Handle an incoming SVE message. Default replies with YES (SVE( ... ) )
    virtual void process_sve_message(TokenMessage &incoming_message);

    // Handle an incoming THX message. Default supplies a simple replacement order if not MBV.
    virtual void process_thx_message(TokenMessage &incoming_message);

    // Handle an incoming TME message.
    virtual void process_tme_message(TokenMessage &incoming_message);

    // Handle an incoming ADM message.
    virtual void process_adm_message(TokenMessage &incoming_message);

    // Handle an incoming NOT( CCD() ) message.
    virtual void
    process_not_ccd_message(TokenMessage &incoming_message, TokenMessage &message_parameters, bool is_new_reconnection);

    // Handle an incoming NOT( TME() ) message.
    virtual void process_not_tme_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( NME() ) message.
    virtual void process_rej_nme_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Get the details to reconnect to the game. Return true if reconnect required, or
    // false if reconnect is not to be attempted. Default implementation uses parameters
    // from the command line
    virtual bool get_reconnect_details(Token &power, int &passcode);

    // Handle an incoming REJ( IAM() ) message.
    virtual void process_rej_iam_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( HLO() ) message.
    virtual void process_rej_hlo_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( NOW() ) message.
    virtual void process_rej_now_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( SCO() ) message.
    virtual void process_rej_sco_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( HST() ) message.
    virtual void process_rej_hst_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( SUB() ) message.
    virtual void process_rej_sub_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( GOF() ) message.
    virtual void process_rej_gof_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( ORD() ) message.
    virtual void process_rej_ord_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( TME() ) message.
    virtual void process_rej_tme_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( DRW() ) message.
    virtual void process_rej_drw_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( SND() ) message.
    virtual void process_rej_snd_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( ADM() ) message.
    virtual void process_rej_adm_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( MIS() ) message.
    virtual void process_rej_mis_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( NOT( GOF() ) ) message.
    virtual void process_rej_not_gof_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming REJ( NOT( DRW() ) ) message.
    virtual void process_rej_not_drw_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( NME() ) message.
    virtual void process_yes_nme_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( OBS() ) message.
    virtual void process_yes_obs_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( IAM() ) message.
    virtual void process_yes_iam_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( GOF() ) message.
    virtual void process_yes_gof_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( TME() ) message.
    virtual void process_yes_tme_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( DRW() ) message.
    virtual void process_yes_drw_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( SND() ) message.
    virtual void process_yes_snd_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( NOT( GOF() ) ) message.
    virtual void process_yes_not_gof_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( NOT( DRW() ) ) message.
    virtual void process_yes_not_drw_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming YES( NOT( SUB() ) ) message.
    virtual void process_yes_not_sub_message(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Handle an incoming NOT message with a parameter we don't expect
    virtual void process_unexpected_not_message(TokenMessage &incoming_message);

    // Handle an incoming REJ message with a parameter we don't expect
    virtual void process_unexpected_rej_message(TokenMessage &incoming_message);

    // Handle an incoming REJ( NOT( ) ) message with a parameter we don't expect
    virtual void process_unexpected_rej_not_message(TokenMessage &incoming_message);

    // Handle an incoming YES message with a parameter we don't expect
    virtual void process_unexpected_yes_message(TokenMessage &incoming_message);

    // Handle an incoming YES( NOT() ) message with a parameter we don't expect
    virtual void process_unexpected_yes_not_message(TokenMessage &incoming_message);

    // The following have a default implementation which does nothing. However, before it is
    // called, a pre-processor does some work to update the internal information ready for
    // the user.

    // Process a HLO message. Power played has already been stored.
    virtual void process_hlo_message(TokenMessage &incoming_message);

    // Process a MAP message. Map name has already been stored, and an MDF sent to the server.
    virtual void process_map_message(TokenMessage &incoming_message);

    // Process a MDF message. Map Definition has already been stored.
    virtual void process_mdf_message(TokenMessage &incoming_message);

    // Process an ORD message. Results have already been stored.
    virtual void process_ord_message(TokenMessage &incoming_message);

    // Process an SCO message. Supply Centre Ownership has already been stored.
    virtual void process_sco_message(TokenMessage &incoming_message);

    // Process a NOW message. Position has already been stored.
    virtual void process_now_message(TokenMessage &incoming_message);

    // Handle an incoming SLO message. Game has already been set to over
    virtual void process_slo_message(TokenMessage &incoming_message);

    // Inform the AI that the press send has failed
    virtual void report_failed_press(bool is_broadcast, TokenMessage &receiving_powers, TokenMessage &press_message);

    // Send press to the server, and add it to the sent press map
    void send_press_to_server(TokenMessage press_to, TokenMessage press_message, bool resend_partial);

    void send_broadcast_to_server(TokenMessage broadcast_message);

    MapAndUnits *m_map_and_units;            // Pointer to the map and units object

    set<Token> m_cd_powers;                    // The powers which are currently CD

private:
    typedef struct {
        TokenMessage original_receiving_powers;
        TokenMessage receiving_powers;
        TokenMessage press_message;
        bool resend_partial;
        bool is_broadcast;
    } SentPressInfo;

    typedef list<SentPressInfo> SentPressList;

    // Process an incoming message
    void process_message(char *&message);

    // Process an incoming rm message
    void process_rm_message(char *message, int message_length);

    // Process an incoming diplomacy message
    void process_diplomacy_message(char *message, int message_length);

    // Process individual messages
    void process_hlo(TokenMessage &incoming_message);

    void process_map(TokenMessage &incoming_message);

    void process_mdf(TokenMessage &incoming_message);

    void process_now(TokenMessage &incoming_message);

    void process_ord(TokenMessage &incoming_message);

    void process_sco(TokenMessage &incoming_message);

    void process_not(TokenMessage &incoming_message);

    void process_rej(TokenMessage &incoming_message);

    void process_yes(TokenMessage &incoming_message);

    void process_rej_not(TokenMessage &incoming_message, TokenMessage &rej_not_parameters);

    void process_yes_not(TokenMessage &incoming_message, TokenMessage &yes_not_parameters);

    void process_slo(TokenMessage &incoming_message);

    void process_ccd(TokenMessage &incoming_message);

    void process_not_ccd(TokenMessage &incoming_message, TokenMessage &message_parameters);

    void process_out(TokenMessage &incoming_message);

    void process_yes_snd(TokenMessage &incoming_message, TokenMessage &message_parameters);

    void process_rej_snd(TokenMessage &incoming_message, TokenMessage &message_parameters);

    // Remove the quotes from a string
    void remove_quotes(std::string &message_string);

    void send_to_reduced_powers(SentPressList::iterator &sent_press_itr, Token &cd_power);

    void check_sent_press_for_missing_power(Token &missing_power);

    void remove_sent_press(TokenMessage &send_message);

    bool m_tcpip_client_started;            // Whether the TCPIP Client has been started successfully

    TokenMessage m_map_message;                // The message containing the map name

    bool m_map_requested;                    // Whether a copy of the map has been requested

    SentPressList m_sent_press;

    COMMAND_LINE_PARAMETERS m_parameters;    // The parameters passed on the command line

    bool extract_parameters(COMMAND_LINE_PARAMETERS &parameters);

public:
    void BaseBot::OnSocketMessage();

    void end_dialog();
};

#endif // _DAN_BASEBOT_H
