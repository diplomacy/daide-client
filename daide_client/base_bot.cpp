/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * BaseBot Code. This is the Base class for all Bots.
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

#include <iostream>
#include <memory>
#include "socket.h"
#include "error_log.h"
#include "map_and_units.h"
#include "token_text_map.h"
#include "ai_client.h"
#include "base_bot.h"

using DAIDE::BaseBot;

BaseBot::BaseBot() {
    log_error("Started");               // not an error, but indicates start of logging; also writes to normal log
    m_map_and_units = MapAndUnits::get_instance();
    m_map_requested = false;
}

BaseBot::~BaseBot() {
    enable_logging(true);
    log_error("Finished");              // not an error, but indicates end of logging; also writes to normal log
    close_logs();
}

bool BaseBot::initialize() {
    COMMAND_LINE_PARAMETERS parameters {};
    const int MAX_COMPUTER_NAME_LEN = 1000;         // Max length of a computer name
    const uint16_t DEFAULT_PORT_NUMBER = 16713;     // Default port number to connect on

    // Extract the parameters
    srand((int) time(nullptr));                     // init random number generator
    extract_parameters(parameters);

    // Store the command line parameters
    m_parameters = parameters;

    // Get the log level
    if (!parameters.log_level_specified) {
#ifdef _DEBUG
        parameters.log_level = 1;
#else
        parameters.log_level = 0;
#endif
    }
    enable_logging(parameters.log_level > 0);

    // Start the TCP/IP
    if (!parameters.name_specified) {
        parameters.server_name = "localhost";
    }
    if (!parameters.port_specified) {
        parameters.port_number = DEFAULT_PORT_NUMBER;
    }
    SetWindowText(main_wnd, BOT_FAMILY " " BOT_GENERATION);

    // Connection failure
    if (!m_socket.Connect(parameters.server_name, parameters.port_number)) {
        log_error("Failed to connect to server");
        return false;
    }

    // Connection success
    send_initial_message_to_server();
    send_nme_or_obs();
    return true;
}

void BaseBot::send_nme_or_obs() {
    TokenMessage obs_message(TOKEN_COMMAND_OBS);
    send_message_to_server(obs_message);
}

void BaseBot::send_initial_message_to_server() {
    Socket::MessagePtr tcp_message;         // Message to send over the link
    DCSP_HST_MESSAGE *tcp_message_header;   // Header of the message
    short *tcp_message_content;

    // FIXME - tcp_message was requesting heap memory space without using a smart pointer
    tcp_message = make_message(4);
    tcp_message_header = get_message_header(tcp_message);
    tcp_message_content = get_message_content<short>(tcp_message);

    // Set message header
    tcp_message_header->type = DCSP_MSG_TYPE_IM;
    tcp_message_header->length = 4;
    tcp_message_content[0] = (short) 1; // version;
    tcp_message_content[1] = (short) 0xDA10; // magic number

    // Send message
    m_socket.PushOutgoingMessage(tcp_message);
}

void BaseBot::send_message_to_server(const TokenMessage &message) {
    int message_length {0};                         // Length of the message in tokens
    Socket::MessagePtr tcp_message;                 // Message to send over the link
    DCSP_HST_MESSAGE *tcp_message_header {nullptr}; // Header of the message
    Token *tcp_message_content;

    log_daide_message(false, message);

    // Get the length of the message
    message_length = message.get_message_length();

    // FIXME - tcp_message was requesting heap memory space without using a smart pointer
    tcp_message = make_message(message_length * 2 + 2);
    tcp_message_header = get_message_header(tcp_message);
    tcp_message_content = get_message_content<Token>(tcp_message);

    // Set message header
    tcp_message_header->type = DCSP_MSG_TYPE_DM;
    tcp_message_header->length = message_length * 2;

    // Send message
    message.get_message(tcp_message_content, message_length + 1);
    m_socket.PushOutgoingMessage(tcp_message);
}

void BaseBot::send_orders_to_server() {
    TokenMessage sub_command = m_map_and_units->build_sub_command();
    if (sub_command.get_message_length() > 1) {
        send_message_to_server(m_map_and_units->build_sub_command());
    }
}

void BaseBot::send_name_and_version_to_server(const std::string &name, const std::string &version) {
    TokenMessage name_message {};
    TokenMessage name_tokens {};
    TokenMessage version_tokens {};

    // Setting and sending
    name_tokens.set_message_from_text("'" + name + "'");
    version_tokens.set_message_from_text("'" + version + "'");
    name_message = TOKEN_COMMAND_NME & name_tokens & version_tokens;
    send_message_to_server(name_message);
}

void BaseBot::disconnect_from_server() {}

void BaseBot::request_map() {
    m_map_requested = true;
    send_message_to_server(TokenMessage(TOKEN_COMMAND_MAP));
}

void BaseBot::process_message(Socket::MessagePtr message) {
    DCSP_HST_MESSAGE *header = get_message_header(message);             // Message Header of the received message
    char* content = get_message_content<char>(message);                 // Message Content of the received message
    unsigned short error_code;                                          // Error code from an error message

    switch (header->type) {

        // Initial Message. Nothing to do.
        case DCSP_MSG_TYPE_IM:
            log_error("Error - Initial message received");
            break;

        // Representation message. Needs handling to update the text map eventually
        case DCSP_MSG_TYPE_RM:
            log("Representation Message received");
            process_rm_message(content, header->length);
            break;

        // Diplomacy Message
        case DCSP_MSG_TYPE_DM:
            log("Diplomacy Message received");
            process_diplomacy_message(content, header->length);
            break;

        // Final Message. Nothing to do.
        case DCSP_MSG_TYPE_FM:
            log("Final Message received");
            break;

        // Error Message. Report
        case DCSP_MSG_TYPE_EM:
            error_code = *get_message_content<unsigned short>(message);
            log_error("Error Message Received. Error Code = %d", error_code);
            break;

        default:
            log_error("Unexpected Message type - %d", header->type);
            break;
    }
}


void BaseBot::process_rm_message(char *message, int message_length) {
    Token token {};                             // The token
    BYTE *token_value {nullptr};                // The byte sequence for the token
    char *token_name {nullptr};                 // The textual version of the token
    TokenTextMap *token_text_map {nullptr};     // The map of tokens to text

    // Nothing to do
    if (message_length == 0) {}

    // Get the token text map
    else {
        token_text_map = TokenTextMap::instance();

        // Clear all the old power and province tokens
        token_text_map->clear_power_and_province_categories();

        // For each entry
        for (int message_ctr = 0; message_ctr < message_length; message_ctr += 6) {
            token_value = reinterpret_cast<BYTE *>(message + message_ctr);
            token_name = message + message_ctr + 2;
            token = Token(token_value[1], token_value[0]);
            token_text_map->add_token(token, token_name);
        }
    }
}

void BaseBot::process_diplomacy_message(char *message, int message_length) {
    TokenMessage incoming_msg {};                       // The incoming message
    Token lead_token {};                                // The first token of the message
    Token *message_tokens {nullptr};                    // The tokens in the incoming message

    // Casting into a token poitner
    message_tokens = reinterpret_cast<Token *>(message);

    // Bad parenthesis
    if (message_tokens[0] == TOKEN_COMMAND_PRN) {
        process_prn_message(message_tokens, message_length);

    } else {
        incoming_msg.set_message(message_tokens, message_length / 2);
        log_daide_message(true, incoming_msg);

        // If the first subtoken is a single message
        if (incoming_msg.submessage_is_single_token(0)) {

            // Act on that token
            lead_token = incoming_msg.get_token(0);

            // FIXME use a switch statement
            // Messages that BaseBot handles initially
            if (lead_token == TOKEN_COMMAND_HLO) {
                process_hlo(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_MAP) {
                process_map(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_MDF) {
                process_mdf(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_NOW) {
                process_now(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_ORD) {
                process_ord(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_SCO) {
                process_sco(incoming_msg);
            }

            // Messages which are broken down further before being passed on
            else if (lead_token == TOKEN_COMMAND_NOT) {
                process_not(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_REJ) {
                process_rej(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_YES) {
                process_yes(incoming_msg);
            }

            // Messages to be handled by Bot immediately
            else if (lead_token == TOKEN_COMMAND_CCD) {
                process_ccd(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_DRW) {
                process_drw_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_FRM) {
                process_frm_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_HUH) {
                process_huh_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_LOD) {
                process_lod_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_MIS) {
                process_mis_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_OFF) {
                process_off_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_OUT) {
                process_out(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_SLO) {
                process_slo_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_SMR) {
                process_smr_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_SVE) {
                process_sve_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_THX) {
                process_thx_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_TME) {
                process_tme_message(incoming_msg);
            } else if (lead_token == TOKEN_COMMAND_ADM) {
                process_adm_message(incoming_msg);
            } else {
                log_error("Unexpected first token in message : %s", incoming_msg.get_message_as_text());
            }

        } else {
            log_error("Illegal message received");
        }
    }
}

// Handle the HLO message. Extract the information we need, then pass on
void BaseBot::process_hlo(const TokenMessage &incoming_msg) {
    TokenMessage power_submessage {};
    TokenMessage passcode_submessage {};

    // Get the submessages
    power_submessage = incoming_msg.get_submessage(1);
    passcode_submessage = incoming_msg.get_submessage(2);

    // Store the details
    m_map_and_units->set_power_played(power_submessage.get_token());
    m_map_and_units->passcode = passcode_submessage.get_token().get_number();
    m_map_and_units->variant = incoming_msg.get_submessage(3);
    m_map_and_units->game_started = true;

    // Pass the message on
    process_hlo_message(incoming_msg);
}

// Handle the MAP message. Store the map name, send the MDF, then pass on
void BaseBot::process_map(const TokenMessage &incoming_msg) {
    TokenMessage mdf_message(TOKEN_COMMAND_MDF);
    TokenMessage name_submessage {};

    // Store the map name
    name_submessage = incoming_msg.get_submessage(1);
    m_map_and_units->map_name = name_submessage.get_message_as_text();
    remove_quotes(m_map_and_units->map_name);

    // Send an MDF
    send_message_to_server(mdf_message);

    // Store the map message
    m_map_message = incoming_msg;

    // Call the users handler function
    process_map_message(incoming_msg);
}

void BaseBot::remove_quotes(std::string &message_string) {
    int start_quote_locn {0};
    int end_quote_locn {0};

    start_quote_locn = message_string.find('\'');
    end_quote_locn = message_string.find_last_of('\'');

    if (end_quote_locn > start_quote_locn) {
        message_string = message_string.substr(start_quote_locn + 1, end_quote_locn - start_quote_locn - 1);
    }
}

// Handle the MDF message. Store the map and pass on
void BaseBot::process_mdf(const TokenMessage &incoming_msg) {
    int set_map_result = m_map_and_units->set_map(incoming_msg);

    if (set_map_result != ADJUDICATOR_NO_ERROR) {
        log_error("Failed to set map - Error location %d", set_map_result);
    }

    process_mdf_message(incoming_msg);

    // If the map wasn't sent by request, then reply to accept the map
    if (!m_map_requested) {
        send_message_to_server(TOKEN_COMMAND_YES & m_map_message);

    // The map was requested following an IAM, so also request a HLO, SCO and NOW.
    } else {
        send_message_to_server(TokenMessage(TOKEN_COMMAND_HLO));
        send_message_to_server(TokenMessage(TOKEN_COMMAND_ORD));
        send_message_to_server(TokenMessage(TOKEN_COMMAND_SCO));
        send_message_to_server(TokenMessage(TOKEN_COMMAND_NOW));
        m_map_requested = false;
    }
}

// Process the NOW message. Store the position and pass on
void BaseBot::process_now(const TokenMessage &incoming_msg) {
    m_map_and_units->set_units(incoming_msg);
    process_now_message(incoming_msg);
}

// Process the ORD message. Store the results and pass on
void BaseBot::process_ord(const TokenMessage &incoming_msg) {
    m_map_and_units->store_result(incoming_msg);
    process_ord_message(incoming_msg);
}

// Process the SCO message. Store the centre ownership and pass on
void BaseBot::process_sco(const TokenMessage &incoming_msg) {
    m_map_and_units->set_ownership(incoming_msg);
    process_sco_message(incoming_msg);
}

// Process the NOT message. Split according to next token
void BaseBot::process_not(const TokenMessage &incoming_msg) {
    TokenMessage not_message = incoming_msg.get_submessage(1);

    // FIXME using switch statement
    if (not_message.get_token(0) == TOKEN_COMMAND_CCD) {
        process_not_ccd(incoming_msg, not_message.get_submessage(1));
    } else if (not_message.get_token(0) == TOKEN_COMMAND_TME) {
        process_not_tme_message(incoming_msg, not_message.get_submessage(1));
    } else {
        process_unexpected_not_message(incoming_msg);
    }
}

// Process the REJ message. Split according to next token
void BaseBot::process_rej(const TokenMessage &incoming_msg) {
    TokenMessage rej_message = incoming_msg.get_submessage(1);

    // FIXME using switch statement
    if (rej_message.get_token(0) == TOKEN_COMMAND_NME) {
        process_rej_nme_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_IAM) {
        process_rej_iam_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_HLO) {
        process_rej_hlo_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_NOW) {
        process_rej_now_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_SCO) {
        process_rej_sco_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_HST) {
        process_rej_hst_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_SUB) {
        process_rej_sub_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_NOT) {             // GOF, DRW
        process_rej_not(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_GOF) {
        process_rej_gof_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_ORD) {
        process_rej_ord_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_TME) {
        process_rej_tme_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_DRW) {
        process_rej_drw_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_SND) {
        process_rej_snd(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_ADM) {
        process_rej_adm_message(incoming_msg, rej_message.get_submessage(1));
    } else if (rej_message.get_token(0) == TOKEN_COMMAND_MIS) {
        process_rej_mis_message(incoming_msg, rej_message.get_submessage(1));
    } else {
        process_unexpected_rej_message(incoming_msg);
    }
}

// Process the REJ(NOT()) message. Split according to next token
void BaseBot::process_rej_not(const TokenMessage &incoming_msg, const TokenMessage &rej_not_params) {
    // FIXME using switch statement
    if (rej_not_params.get_token(0) == TOKEN_COMMAND_GOF) {
        process_rej_not_gof_message(incoming_msg, rej_not_params.get_submessage(1));
    } else if (rej_not_params.get_token(0) == TOKEN_COMMAND_DRW) {
        process_rej_not_drw_message(incoming_msg, rej_not_params.get_submessage(1));
    } else {
        process_unexpected_rej_not_message(incoming_msg);
    }
}

// Process the YES message. Split according to next token
void BaseBot::process_yes(const TokenMessage &incoming_msg) {
    TokenMessage yes_message = incoming_msg.get_submessage(1);

    // FIXME use switch statement
    if (yes_message.get_token(0) == TOKEN_COMMAND_NME) {
        process_yes_nme_message(incoming_msg, yes_message.get_submessage(1));
    } else if (yes_message.get_token(0) == TOKEN_COMMAND_OBS) {
        process_yes_obs_message(incoming_msg, yes_message.get_submessage(1));
    } else if (yes_message.get_token(0) == TOKEN_COMMAND_IAM) {
        process_yes_iam_message(incoming_msg, yes_message.get_submessage(1));
    } else if (yes_message.get_token(0) == TOKEN_COMMAND_NOT) {                 // GOF, DRW
        process_yes_not(incoming_msg, yes_message.get_submessage(1));
    } else if (yes_message.get_token(0) == TOKEN_COMMAND_GOF) {
        process_yes_gof_message(incoming_msg, yes_message.get_submessage(1));
    } else if (yes_message.get_token(0) == TOKEN_COMMAND_TME) {
        process_yes_tme_message(incoming_msg, yes_message.get_submessage(1));
    } else if (yes_message.get_token(0) == TOKEN_COMMAND_DRW) {
        process_yes_drw_message(incoming_msg, yes_message.get_submessage(1));
    } else if (yes_message.get_token(0) == TOKEN_COMMAND_SND) {
        process_yes_snd(incoming_msg, yes_message.get_submessage(1));
    } else {
        process_unexpected_yes_message(incoming_msg);
    }
}

// Process the YES(NOT()) message. Split according to next token
void BaseBot::process_yes_not(const TokenMessage &incoming_msg, const TokenMessage &yes_not_params) {

    // FIXME use switch staetment
    if (yes_not_params.get_token(0) == TOKEN_COMMAND_GOF) {
        process_yes_not_gof_message(incoming_msg, yes_not_params.get_submessage(1));
    } else if (yes_not_params.get_token(0) == TOKEN_COMMAND_DRW) {
        process_yes_not_drw_message(incoming_msg, yes_not_params.get_submessage(1));
    } else if (yes_not_params.get_token(0) == TOKEN_COMMAND_SUB) {
        process_yes_not_sub_message(incoming_msg, yes_not_params.get_submessage(1));
    } else {
        process_unexpected_yes_not_message(incoming_msg);
    }
}

// Handle an incoming FRM message. Default version replies with HUH( message ). TRY().
void BaseBot::process_frm_message(const TokenMessage &incoming_msg) {
    Token from_power {};
    TokenMessage huh_message {};
    TokenMessage try_message {};
    TokenMessage press_message {};
    TokenMessage message_id {};

    from_power = message_id.get_token();
    message_id = incoming_msg.get_submessage(1);
    press_message = incoming_msg.get_submessage(3);

    // Replying HUH TRY
    if ((press_message.get_token(0) != TOKEN_COMMAND_HUH) && (press_message.get_token(0) != TOKEN_PRESS_TRY)) {
        huh_message = TOKEN_COMMAND_SND & from_power & (TOKEN_COMMAND_HUH & (TOKEN_PARAMETER_ERR + press_message));
        try_message = TOKEN_COMMAND_SND & from_power & (TOKEN_PRESS_TRY & try_message);

        send_message_to_server(huh_message);
        send_message_to_server(try_message);
    }
}

// Handle an incoming THX message. Default supplies a simple replacement order if not MBV.
void BaseBot::process_thx_message(const TokenMessage &incoming_msg) {
    bool send_new_order {false};            // Whether to send a new order
    TokenMessage order {};                  // The order submitted
    Token note {};                          // The order note returned
    TokenMessage unit {};                   // The unit ordered
    TokenMessage new_order {};              // The replacement order to submit

    order = incoming_msg.get_submessage(1);
    unit = order.get_submessage(0).enclose();
    note = incoming_msg.get_submessage(2).get_token();

    // Everything is good. Nothing to do.
    if (note == TOKEN_ORDER_NOTE_MBV) {}

    // Illegal movement order. Replace with a hold order.
    if ((note == TOKEN_ORDER_NOTE_FAR)
            || (note == TOKEN_ORDER_NOTE_NSP)
            || (note == TOKEN_ORDER_NOTE_NSU)
            || (note == TOKEN_ORDER_NOTE_NAS)
            || (note == TOKEN_ORDER_NOTE_NSF)
            || (note == TOKEN_ORDER_NOTE_NSA)) {

        new_order = unit + TOKEN_ORDER_HLD;
        send_new_order = true;
    }

    // Illegal retreat order. Replace with a disband order
    if (note == TOKEN_ORDER_NOTE_NVR) {
        new_order = unit + TOKEN_ORDER_DSB;
        send_new_order = true;
    }

    // Illegal build order. Replace with a waive order
    if ((note == TOKEN_ORDER_NOTE_YSC)
            || (note == TOKEN_ORDER_NOTE_ESC)
            || (note == TOKEN_ORDER_NOTE_HSC)
            || (note == TOKEN_ORDER_NOTE_NSC)
            || (note == TOKEN_ORDER_NOTE_CST)) {

        new_order = unit.get_submessage(0) + TOKEN_ORDER_WVE;
        send_new_order = true;
    }

    // Illegal order, but not much we can do about it
    if ((note == TOKEN_ORDER_NOTE_NYU) || (note == TOKEN_ORDER_NOTE_NRS)) {}

    // Order wasn't needed in the first place!
    if ((note == TOKEN_ORDER_NOTE_NRN) || (note == TOKEN_ORDER_NOTE_NMB) || (note == TOKEN_ORDER_NOTE_NMR)) {}

    // Sending new order
    if ((send_new_order) && (new_order != order)) {
        log_error("THX returned %s for order '%s'. Replacing with '%s'",
                  incoming_msg.get_submessage(2).get_message_as_text(),
                  order.get_message_as_text(),
                  new_order.get_message_as_text());
        send_message_to_server(new_order);

    // Logging error if no new orders were sent
    } else if (note != TOKEN_ORDER_NOTE_MBV) {
        log_error("THX returned %s for order '%s'. No replacement order sent.",
                  incoming_msg.get_submessage(2).get_message_as_text(),
                  order.get_message_as_text());
    }
}

// Handle an incoming REJ( NME() ) message.
void BaseBot::process_rej_nme_message(const TokenMessage & /*incoming_msg*/, const TokenMessage & /*msg_params*/) {
    bool attempt_reconnect {false};         // Whether to try and reconnect
    int passcode {0};                       // The passcode to reconnect as
    Token power_token {0};                  // The token for the power to reconnect as
    Token passcode_token {};                // The passcode as a token

    attempt_reconnect = get_reconnect_details(power_token, passcode);

    // Send an IAM message
    if (attempt_reconnect) {
        passcode_token.set_number(passcode);
        send_message_to_server(TOKEN_COMMAND_IAM & power_token & passcode_token);

    // Disconnect
    } else {
        disconnect_from_server();
        end_dialog();
    }
}

// Determine whether to try and reconnect to game. Default uses values passed on command line.
bool BaseBot::get_reconnect_details(const Token &power, int &passcode) {
    if (m_parameters.reconnection_specified) {
        power = TokenTextMap::instance()->m_text_to_token_map[m_parameters.reconnect_power];
        passcode = m_parameters.reconnect_passcode;
    }
    return m_parameters.reconnection_specified;
}

void BaseBot::send_press_to_server(const TokenMessage &press_to,
                                   const TokenMessage &press_message,
                                   bool resend_partial) {

    SentPressInfo sent_press_record {};

    sent_press_record.original_receiving_powers = press_to;
    sent_press_record.receiving_powers = press_to;
    sent_press_record.press_message = press_message;
    sent_press_record.resend_partial = resend_partial;
    sent_press_record.is_broadcast = false;

    // Recording and sending
    m_sent_press.push_back(sent_press_record);
    send_message_to_server(TOKEN_COMMAND_SND & press_to & press_message);
}

void BaseBot::send_broadcast_to_server(const TokenMessage &broadcast_message) {
    TokenMessage receiving_powers {};
    SentPressInfo sent_press_record {};

    for (int power_ctr = 0; power_ctr < m_map_and_units->number_of_powers; power_ctr++) {
        if ((m_map_and_units->power_played.get_subtoken() != power_ctr)
                && (m_cd_powers.find(Token(CATEGORY_POWER, power_ctr)) == m_cd_powers.end())) {

            receiving_powers = receiving_powers + Token(CATEGORY_POWER, power_ctr);
        }
    }

    sent_press_record.original_receiving_powers = receiving_powers;
    sent_press_record.receiving_powers = receiving_powers;
    sent_press_record.press_message = broadcast_message;
    sent_press_record.resend_partial = true;
    sent_press_record.is_broadcast = true;

    // Recording and sending
    m_sent_press.push_back(sent_press_record);
    send_message_to_server(TOKEN_COMMAND_SND & receiving_powers & broadcast_message);
}

void BaseBot::process_ccd(const TokenMessage &incoming_msg) {
    bool is_new_disconnection {false};
    Token cd_power {};
    std::string message;

    cd_power = incoming_msg.get_submessage(1).get_token();
    check_sent_press_for_missing_power(cd_power);
    if (m_cd_powers.find(cd_power) == m_cd_powers.end()) {
        m_cd_powers.insert(cd_power);
        is_new_disconnection = true;
    }
    process_ccd_message(incoming_msg, is_new_disconnection);
}

void BaseBot::process_not_ccd(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {
    bool is_new_reconnection {false};
    Token cd_power {};

    cd_power = msg_params.get_token();
    if (m_cd_powers.find(cd_power) != m_cd_powers.end()) {
        m_cd_powers.erase(cd_power);
        is_new_reconnection = true;
    }
    process_not_ccd_message(incoming_msg, msg_params, is_new_reconnection);
}

void BaseBot::process_out(const TokenMessage &incoming_msg) {
    Token out_power {};
    out_power = incoming_msg.get_submessage(1).get_token();
    check_sent_press_for_missing_power(out_power);
    process_out_message(incoming_msg);
}

void BaseBot::check_sent_press_for_missing_power(Token &missing_power) {
    bool missing_power_found {false};
    TokenMessage receiving_powers {};

    for (auto &m_sent_msg : m_sent_press) {
        receiving_powers = m_sent_msg.receiving_powers;

        for (int power_ctr = 0; power_ctr < receiving_powers.get_message_length(); power_ctr++) {
            if (receiving_powers.get_token(power_ctr) == missing_power) {

                // Resending the message
                if (m_sent_msg.resend_partial) {
                    send_to_reduced_powers(m_sent_msg, missing_power);
                    missing_power_found = true;

                // Logging the failure
                } else {
                    report_failed_press(m_sent_msg.is_broadcast,
                                        m_sent_msg.original_receiving_powers,
                                        m_sent_msg.press_message);
                    missing_power_found = true;
                }
            }
            if (!missing_power_found) { break; }
        }
    }
}

void BaseBot::send_to_reduced_powers(SentPressInfo &sent_press, const Token &cd_power) {
    TokenMessage receiving_powers {};
    TokenMessage reduced_powers {};

    receiving_powers = sent_press.receiving_powers;
    for (int power_ctr = 0; power_ctr < receiving_powers.get_message_length(); power_ctr++) {
        if (receiving_powers.get_token(power_ctr) != cd_power) {
            reduced_powers = reduced_powers + receiving_powers.get_token(power_ctr);
        }
    }

    // Resending
    sent_press.receiving_powers = reduced_powers;
    send_message_to_server(TOKEN_COMMAND_SND & reduced_powers & sent_press.press_message);
}

void BaseBot::process_yes_snd(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {
    TokenMessage send_message {};
    send_message = incoming_msg.get_submessage(1);
    remove_sent_press(send_message);
    process_yes_snd_message(incoming_msg, msg_params);
}

void BaseBot::process_rej_snd(const TokenMessage &incoming_msg, const TokenMessage &msg_params) {
    TokenMessage send_message {};
    send_message = incoming_msg.get_submessage(1);
    remove_sent_press(send_message);
    process_rej_snd_message(incoming_msg, msg_params);
}

void BaseBot::remove_sent_press(const TokenMessage &send_message) {
    TokenMessage to_powers {};
    TokenMessage press_msg {};

    to_powers = send_message.get_submessage(1);
    press_msg = send_message.get_submessage(2);

    // Remove the message from the sent press
    auto sent_press_itr = m_sent_press.begin();

    while (sent_press_itr != m_sent_press.end()) {
        auto sent_press_itr_copy = sent_press_itr;
        sent_press_itr++;

        if ((sent_press_itr_copy->receiving_powers == to_powers) && (sent_press_itr_copy->press_message == press_msg)) {
            m_sent_press.erase(sent_press_itr_copy);
        }
    }
}

bool BaseBot::extract_parameters(COMMAND_LINE_PARAMETERS &parameters) {
    bool extracted_ok {true};               // Whether the parameters were OK
    int search_start {0};                   // Position to start searching command line
    int param_start {0};                    // Start of the next parameter
    int param_end {0};                      // End of the next parameter
    char param_token {};                    // The token specifying the parameter type
    std::string parameter;                  // The parameter

    parameters.ip_specified = false;
    parameters.name_specified = false;
    parameters.port_specified = false;
    parameters.log_level_specified = false;
    parameters.reconnection_specified = false;

    // Getting parameters
    std::string m_command_line = GetCommandLineA();

    // Strip the program name off the command line
    // Program name is in quotes.
    if (m_command_line[0] == '"') {
        param_start = m_command_line.find('"', 1);
        if (param_start != std::string::npos) {
            m_command_line = m_command_line.substr(param_start + 1);
        }

    // Program name is not quoted, so is terminated by a space
    } else {
        param_start = m_command_line.find(' ');
        if (param_start != std::string::npos) {
            m_command_line = m_command_line.substr(param_start);
        }
    }

    // Parsing flags
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

        switch (tolower(param_token)) {
            case 's':
                parameters.name_specified = true;
                parameters.server_name = parameter;
                break;

            case 'i':
                parameters.ip_specified = true;
                parameters.server_name = parameter;
                break;

            case 'p':
                parameters.port_specified = true;
                parameters.port_number = stoi(parameter);
                break;

            case 'l':
                parameters.log_level_specified = true;
                parameters.log_level = stoi(parameter);
                break;

            case 'r':
                if (parameter[3] == ':') {
                    parameters.reconnection_specified = true;
                    parameters.reconnect_power = parameter.substr(0, 3);
                    for (auto &c : parameters.reconnect_power) { c = toupper(c); }
                    parameters.reconnect_passcode = stoi(parameter.substr(4));
                } else {
                    std::cout << "-r should be followed by 'POW:passcode'\nPOW should be three characters" << std::endl;
                }
                break;

            default:
                std::cout << "Usage: " << BOT_FAMILY << ".exe "
                          << "[-sServerName|-iIPAddress] [-pPortNumber] [-lLogLevel] [-rPOW:passcode]" << std::endl;
                extracted_ok = false;
        }
        param_start = m_command_line.find('-', search_start);
    }

    if ((parameters.ip_specified) && (parameters.name_specified)) {
        std::cerr << "You must not specify Server name and IP address" << std::endl;
        extracted_ok = false;
    }

    return extracted_ok;
}

void BaseBot::OnSocketMessage() {
    // Process a DAIDE event: there are 1 or more unprocessed incoming DAIDE messages, in order of arrival.
    // Ensure all are processed, in order.
    // Assumes messages were queued in the same thread.

    // Get and remove next DAIDE message to be processed.
    // Triggers recall if more messages remain.
    // (More responsive than processing all such messages at once, without first checking for more urgent events.)
    Socket::MessagePtr incomingMessage = m_socket.PullIncomingMessage();

    process_message(incomingMessage);
}

void BaseBot::end_dialog() {
    PostMessage(main_wnd, WM_CLOSE, 0, 0);
}
