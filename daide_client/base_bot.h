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

#ifndef _DAIDE_CLIENT_DAIDE_CLIENT_BASE_BOT_H
#define _DAIDE_CLIENT_DAIDE_CLIENT_BASE_BOT_H

#include "ai_client_types.h"
#include "token_message.h"
#include "map_and_units.h"
#include "error_log.h"
#include "socket.h"

namespace DAIDE {

using DCSP_HST_MESSAGE = MessageHeader;

// DAIDE message types.
#define DCSP_MSG_TYPE_IM        0       // Initial Message
#define DCSP_MSG_TYPE_RM        1       // Representation Message
#define DCSP_MSG_TYPE_DM        2       // Diplomacy Message
#define DCSP_MSG_TYPE_FM        3       // Final Message
#define DCSP_MSG_TYPE_EM        4       // Error Message

class MapAndUnits;            // The map and units class

// BaseBot : The base class for all Bots

class BaseBot {
public:
    Socket m_socket;

    BaseBot();
    BaseBot(const BaseBot &other) = default;                // Copy constructor
    BaseBot(BaseBot &&rhs) = default;                       // Move constructor
    virtual ~BaseBot();

    BaseBot& operator=(const BaseBot &other) = default;     // Copy Assignment
    BaseBot& operator=(BaseBot &&rhs) = default;            // Move Assignment

    // Initialize the AI. May be overridden, but should call the base class version at the top of the derived version
    // if it is
    virtual bool initialize(const std::string &command_line_a);

    bool is_active() const;

protected:
    // Useful utility functions

    // Send initial message to the server
    void send_initial_message_to_server();

    // Send a message to the server
    void send_message_to_server(const TokenMessage &message);

    // Send the set orders to the server (see set_..._order() in MapAndUnits)
    void send_orders_to_server();

    // Send a name and version to the server
    void send_name_and_version_to_server(const std::string &name, const std::string &version);

    // Disconnect from the server
    void disconnect_from_server();

    // Request a copy of the map. Gets a MAP and MDF, but doesn't YES(MAP()) in response
    // Done automatically if joining a game via HLO or OBS, but not if via IAM (since you
    // may just be rejoining following connection loss).
    void request_map();

    // Overrideables
    // The following virtual functions have a default implementation, but you may completely override them.

    // Send the NME or OBS message to server. Default sends OBS.
    virtual void send_nme_or_obs();

    // Handle an incoming CCD message - Ignored by default
    virtual void process_ccd_message(const TokenMessage &incoming_msg, bool is_new_disconnection) {};

    // Handle an incoming DRW message. Default sets the game to over
    virtual void process_drw_message(const TokenMessage & /*incoming_msg*/) { m_map_and_units->game_over = true; };

    // Handle an incoming FRM message. Default version replies with HUH( message ). TRY().
    virtual void process_frm_message(const TokenMessage &incoming_msg);

    // Handle an incoming HUH message. Default logs it
    virtual void process_huh_message(const TokenMessage &incoming_msg) {
        log_error("HUH message received : %s", incoming_msg.get_message_as_text());
    };

    // Handle an incoming LOD message. Default replies with REJ (LOD (...) )
    virtual void process_lod_message(const TokenMessage &incoming_msg) {
        send_message_to_server(TOKEN_COMMAND_REJ & incoming_msg);
    };

    // Handle an incoming MIS message.
    virtual void process_mis_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming OFF message. Default disconnects from server and exits
    virtual void process_off_message(const TokenMessage & /*incoming_msg*/) { disconnect_from_server(); };

    // Handle an incoming OUT message.
    virtual void process_out_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming PRN message. Default logs it
    virtual void process_prn_message(const Token *incoming_msg, int /*message_length*/) {
        log_error("PRN message received. Starts %.4X %.4X %.4X %.4X",
                  incoming_msg[0], incoming_msg[1], incoming_msg[2], incoming_msg[3]);
    }

    // Handle an incoming SMR message.
    virtual void process_smr_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming SVE message. Default replies with YES (SVE( ... ) )
    virtual void process_sve_message(const TokenMessage &incoming_msg) {
        send_message_to_server(TOKEN_COMMAND_YES & incoming_msg);
    };

    // Handle an incoming THX message. Default supplies a simple replacement order if not MBV.
    virtual void process_thx_message(const TokenMessage &incoming_msg);

    // Handle an incoming TME message.
    virtual void process_tme_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming ADM message.
    virtual void process_adm_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming NOT( CCD() ) message.
    virtual void process_not_ccd_message(const TokenMessage &incoming_msg,
                                         const TokenMessage &msg_params,
                                         bool is_new_reconnection) {};

    // Handle an incoming NOT( TME() ) message.
    virtual void process_not_tme_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming REJ( NME() ) message.
    virtual void process_rej_nme_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params);

    // Get the details to reconnect to the game. Return true if reconnect required, or false if reconnect is not to
    // be attempted. Default implementation uses parameters from the command line
    virtual bool get_reconnect_details(Token &power, int &passcode);

    // Handle an incoming REJ( IAM() ) message.
    virtual void process_rej_iam_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( IAM() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( HLO() ) message. Default logs it
    void process_rej_hlo_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( HLO() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( NOW() ) message. Default logs it
    void process_rej_now_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( NOW() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( SCO() ) message. Default logs it
    void process_rej_sco_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( SCO() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( HST() ) message. Default logs it
    void process_rej_hst_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( HST() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( SUB() ) message. Default logs it
    void process_rej_sub_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( SUB() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( GOF() ) message. Default logs it
    void process_rej_gof_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( GOF() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( ORD() ) message. Default logs it
    void process_rej_ord_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( ORD() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( TME() ) message. Default logs it
    void process_rej_tme_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( TME() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( DRW() ) message. Default logs it
    void process_rej_drw_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( DRW() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( SND() ) message. Default logs it
    void process_rej_snd_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( SND() ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( ADM() ) message. Default logs it
    virtual void process_rej_adm_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( ADM() ) message received : %s", incoming_msg.get_message_as_text());
    };

    // Handle an incoming REJ( MIS() ) message. Default logs it
    virtual void process_rej_mis_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( MIS() ) message received : %s", incoming_msg.get_message_as_text());
    };

    // Handle an incoming REJ( NOT( GOF() ) ) message. Default logs it
    void process_rej_not_gof_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( NOT( GOF() ) ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming REJ( NOT( DRW() ) ) message. Default logs it
    void process_rej_not_drw_message(const TokenMessage &incoming_msg, const TokenMessage & /*msg_params*/) {
        log_error("REJ( NOT( DRW() ) ) message received : %s", incoming_msg.get_message_as_text());
    }

    // Handle an incoming YES( NME() ) message.
    virtual void process_yes_nme_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming YES( OBS() ) message.
    virtual void process_yes_obs_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming YES( IAM() ) message.
    virtual void process_yes_iam_message(const TokenMessage & /*incoming_msg*/, const TokenMessage & /*msg_params*/) {
        request_map();
    }

    // Handle an incoming YES( GOF() ) message.
    virtual void process_yes_gof_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming YES( TME() ) message.
    virtual void process_yes_tme_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming YES( DRW() ) message.
    virtual void process_yes_drw_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming YES( SND() ) message.
    virtual void process_yes_snd_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming YES( NOT( GOF() ) ) message.
    virtual void process_yes_not_gof_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming YES( NOT( DRW() ) ) message.
    virtual void process_yes_not_drw_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming YES( NOT( SUB() ) ) message.
    virtual void process_yes_not_sub_message(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {};

    // Handle an incoming NOT message with a parameter we don't expect
    virtual void process_unexpected_not_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming REJ message with a parameter we don't expect
    virtual void process_unexpected_rej_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming REJ( NOT( ) ) message with a parameter we don't expect
    virtual void process_unexpected_rej_not_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming YES message with a parameter we don't expect
    virtual void process_unexpected_yes_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming YES( NOT() ) message with a parameter we don't expect
    virtual void process_unexpected_yes_not_message(const TokenMessage &incoming_msg) {};

    // The following have a default implementation which does nothing. However, before it is called,
    // a pre-processor does some work to update the internal information ready for the user.

    // Process a HLO message. Power played has already been stored.
    virtual void process_hlo_message(const TokenMessage &incoming_msg) {};

    // Process a MAP message. Map name has already been stored, and an MDF sent to the server.
    virtual void process_map_message(const TokenMessage &incoming_msg) {};

    // Process a MDF message. Map Definition has already been stored.
    virtual void process_mdf_message(const TokenMessage &incoming_msg) {};

    // Process an ORD message. Results have already been stored.
    virtual void process_ord_message(const TokenMessage &incoming_msg) {};

    // Process an SCO message. Supply Centre Ownership has already been stored.
    virtual void process_sco_message(const TokenMessage &incoming_msg) {};

    // Process a NOW message. Position has already been stored.
    virtual void process_now_message(const TokenMessage &incoming_msg) {};

    // Handle an incoming SLO message. Game has already been set to over
    virtual void process_slo_message(const TokenMessage &incoming_msg) {};

    // Inform the AI that the press send has failed
    virtual void report_failed_press(bool is_broadcast,
                                     const TokenMessage &receiving_powers,
                                     const TokenMessage &press_message) {};

    // Send press to the server, and add it to the sent press map
    void send_press_to_server(const TokenMessage &press_to, const TokenMessage &press_message, bool resend_partial);

    void send_broadcast_to_server(const TokenMessage &broadcast_message);

    MapAndUnits *m_map_and_units;               // Pointer to the map and units object
    std::set<Token> m_cd_powers;                // The powers which are currently CD
    bool m_is_active {false};

private:
    using SentPressInfo = struct {
        TokenMessage original_receiving_powers;
        TokenMessage receiving_powers;
        TokenMessage press_message;
        bool resend_partial;
        bool is_broadcast;
    };

    using SentPressList = std::list<SentPressInfo>;

    // Process an incoming message
    void process_message(Socket::MessagePtr message);

    // Process an incoming rm message
    static void process_rm_message(char *message, int message_length);

    // Process an incoming diplomacy message
    void process_diplomacy_message(char *message, int message_length);

    // Process individual messages
    void process_hlo(const TokenMessage &incoming_msg);

    void process_map(const TokenMessage &incoming_msg);

    void process_mdf(const TokenMessage &incoming_msg);

    void process_now(const TokenMessage &incoming_msg);

    void process_ord(const TokenMessage &incoming_msg);

    void process_sco(const TokenMessage &incoming_msg);

    void process_not(const TokenMessage &incoming_msg);

    void process_rej(const TokenMessage &incoming_msg);

    void process_yes(const TokenMessage &incoming_msg);

    void process_rej_not(const TokenMessage &incoming_msg, const TokenMessage &rej_not_params);

    void process_yes_not(const TokenMessage &incoming_msg, const TokenMessage &yes_not_params);

    void process_slo(const TokenMessage &incoming_msg) {
        m_map_and_units->game_over = true;
        process_slo_message(incoming_msg);
    };

    void process_ccd(const TokenMessage &incoming_msg);

    void process_not_ccd(const TokenMessage &incoming_msg, const TokenMessage &msg_params);

    void process_out(const TokenMessage &incoming_msg);

    void process_yes_snd(const TokenMessage &incoming_msg, const TokenMessage &msg_params);

    void process_rej_snd(const TokenMessage &incoming_msg, const TokenMessage &msg_params);

    // Remove the quotes from a string
    static void remove_quotes(std::string &message_string);

    void send_to_reduced_powers(SentPressInfo &sent_press, const Token &cd_power);

    void check_sent_press_for_missing_power(Token &missing_power);

    void remove_sent_press(const TokenMessage &send_message);

    TokenMessage m_map_message;                 // The message containing the map name

    bool m_map_requested;                       // Whether a copy of the map has been requested

    SentPressList m_sent_press;

    COMMAND_LINE_PARAMETERS m_parameters;       // The parameters passed on the command line

    bool extract_parameters(const std::string &command_line_a, COMMAND_LINE_PARAMETERS &parameters);

public:
    void OnSocketMessage();

    void stop();
};

} // namespace DAIDE

#endif // _DAIDE_CLIENT_DAIDE_CLIENT_BASE_BOT_H
