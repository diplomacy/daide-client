/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * TokenMessage Class Header.
 *
 * (C) David Norman 2002 david@ellought.demon.co.uk
 *
 * This software may be reused for non-commercial purposes without charge, and
 * without notifying the author. Use of any part of this software for commercial 
 * purposes without permission from the Author is prohibited.
 *
 * Release 8~2
 **/

#ifndef _DAIDE_CLIENT_DAIDE_CLIENT_TOKEN_MESSAGE_H
#define _DAIDE_CLIENT_DAIDE_CLIENT_TOKEN_MESSAGE_H

#include "types.h"
#include "tokens.h"

namespace DAIDE {

class TokenMessage {
public:
    // Construct as a blank message
    TokenMessage();

    // Construct with a message as specified. Message must be terminated
    explicit TokenMessage(const Token *message);

    // Construct with a message as specified.
    TokenMessage(const Token *message, int message_length);

    // Construct to contain a single token
    TokenMessage(const Token &token);

    // Copy another token message
    TokenMessage(const TokenMessage &message_to_copy);

    // Destruct the message
    ~TokenMessage();

    // Copy the message
    TokenMessage &operator=(const TokenMessage &message_to_copy);

    // Get the message back in raw format
    bool get_message(Token message[], int buffer_length) const;

    // Get the length of the message
    int get_message_length() const { return m_message_length; };

    // Find out if the message is a single token
    bool is_single_token() const { return (m_message_length == 1); };

    // Find out if the message contains submessages or just individual tokens
    bool contains_submessages() const { return (m_message_length != m_submessage_count); };

    // Get the first token (if a single token, it is the only one
    Token get_token() const { return m_message[0]; };

    // Get a token by index
    Token get_token(int index) const;

    // Get the number of submessages (a submessage is one token, or ( ... ) )
    int get_submessage_count() const;

    // Get a submessage
    TokenMessage get_submessage(int submessage_index) const;

    // Get the number of tokens in the message before a given submessage
    int get_submessage_start(int submessage_index) const;

    // Determine whether a submessage is a single token
    bool submessage_is_single_token(int submessage_index);

    // Set the message. Message must be terminated. Returns location of error or ADJUDICATOR_NO_ERROR
    int set_message(const Token *message);

    // Set the message. Returns location of error or ADJUDICATOR_NO_ERROR
    int set_message(const Token *message, int message_length);

    // Set the message from a string. Returns location of error or ADJUDICATOR_NO_ERROR
    int set_message_from_text(const std::string &text);

    // Set the message as a string of ASCII category tokens
    void set_message_as_ascii(const std::string &text);

    // Get the message as a string
    std::string get_message_as_text() const;

    // Enclose the message in brackets and return
    TokenMessage enclose() const;

    // Enclose the message in brackets and update this message
    void enclose_this();

    // Concatenate two messages together. Treat the second as a submessage
    TokenMessage operator&(const TokenMessage &other_message);

    // Concatenate two messages together directly.
    TokenMessage operator+(const TokenMessage &other_message);

    // Concatenate a token to a message. Treat the token as a submessage
    TokenMessage operator&(const Token &token);

    // Concatenate a token to a message directly.
    TokenMessage operator+(const Token &token);

    // Compare the TokenMessages
    bool operator<(const TokenMessage &other) const;

    bool operator==(const TokenMessage &other) const;

    bool operator!=(const TokenMessage &other) const;

    enum { NO_MESSAGE = -1 };

private:
    void find_submessages() const;

    Token *m_message;                       // The message
    int m_message_length;                   // Number of tokens in the message
    int *m_submessage_starts;               // The start of each submessage
    int m_submessage_count;                 // The number of submessages
};

} // namespace DAIDE

#endif // _DAIDE_CLIENT_DAIDE_CLIENT_TOKEN_MESSAGE_H
