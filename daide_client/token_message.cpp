/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * TokenMessage Class. Handles a tokenised message and provides functionality to
 * manipulate and break up the message.
 *
 * (C) David Norman 2002 david@ellought.demon.co.uk
 *
 * This software may be reused for non-commercial purposes without charge, and
 * without notifying the author. Use of any part of this software for commercial
 * purposes without permission from the Author is prohibited.
 *
 * Release 8~3
 **/

#include <sstream>
#include <cstring>
#include "token_message.h"
#include "token_text_map.h"

using DAIDE::Token;
using DAIDE::TokenMessage;

// No-Arg Constructor - Set the message to blank
TokenMessage::TokenMessage() :
    m_message {nullptr},
    m_message_length {NO_MESSAGE},
    m_submessage_starts {nullptr},
    m_submessage_count {NO_MESSAGE} {}

TokenMessage::TokenMessage(const Token *message) : TokenMessage() {
    set_message(message);
}

TokenMessage::TokenMessage(const Token *message, int message_length) : TokenMessage() {
    set_message(message, message_length);
}

TokenMessage::TokenMessage(const Token &token) : TokenMessage() {
    set_message(&token, 1);
}

TokenMessage::TokenMessage(const TokenMessage &message_to_copy) : TokenMessage() {
    if (message_to_copy.m_message != nullptr) {
        set_message(message_to_copy.m_message, message_to_copy.m_message_length);
    }
}

TokenMessage::~TokenMessage() {
    // FIXME use smart pointers rather than delete
    if (m_message != nullptr) {
        delete[] m_message;
        m_message = nullptr;
    }

    if (m_submessage_starts != nullptr) {
        delete[] m_submessage_starts;
        m_submessage_starts = nullptr;
    }
}

TokenMessage &TokenMessage::operator=(const TokenMessage &message_to_copy) {
    // FIXME use smart pointers rather than delete
    // Delete any old message
    if (m_message != nullptr) {
        delete[] m_message;
        m_message = nullptr;
    }

    if (m_submessage_starts != nullptr) {
        delete[] m_submessage_starts;
        m_submessage_starts = nullptr;
    }

    m_message_length = NO_MESSAGE;
    m_submessage_count = NO_MESSAGE;

    // Copy the message
    if (message_to_copy.m_message != nullptr) {
        set_message(message_to_copy.m_message, message_to_copy.m_message_length);
    }

    return *this;
}

/**
 * @param message - Buffer to copy message into
 * @param buffer_length - Size of buffer available
 * @return Whether the message was copied
 */
bool TokenMessage::get_message(Token message[], int buffer_length) const {
    // No message, or not enough buffer to copy
    if (m_message == nullptr || buffer_length < m_message_length) { return false; }

    // Copying the data
    // FIXME - Use something else than memcpy
    memcpy(message, m_message, (m_message_length + 1) * sizeof(Token));
    return true;
}

Token TokenMessage::get_token(int index) const {
    Token token {};
    if ((index >= m_message_length) || (index < 0)) { token = TOKEN_END_OF_MESSAGE; }
    else { token = m_message[index]; }
    return token;
}

int TokenMessage::get_submessage_count() const {
    int submessage_count {0};
    if (m_submessage_count != NO_MESSAGE) { submessage_count = m_submessage_count; }
    return submessage_count;
}

TokenMessage TokenMessage::get_submessage(int submessage_index) const {
    TokenMessage submessage {};             // The submessage to return
    int submessage_length {0};              // Length of the submessage to copy

    if (m_message != nullptr) {
        // TODO: find_submessages() has been moved to set_message(). Make sure this brings no bugs
        // if (m_submessage_starts == nullptr) { find_submessages(); }

        if (submessage_index < m_submessage_count) {
            // Find the length of the submessage
            submessage_length = m_submessage_starts[submessage_index + 1] - m_submessage_starts[submessage_index];

            // If it is one token, then just copy the token
            if (submessage_length == 1) {
                submessage.set_message(&(m_message[m_submessage_starts[submessage_index]]), 1);

            // Copy the submessage, but leave off the start and end brackets
            } else {
                submessage.set_message(&(m_message[m_submessage_starts[submessage_index] + 1]), submessage_length - 2);
            }
        }
    }
    return submessage;
}

int TokenMessage::get_submessage_start(int submessage_index) const {
    int submessage_start {NO_MESSAGE};

    if ((submessage_index < m_submessage_count) && (submessage_index >= 0)) {
        // TODO: find_submessages() has been moved to set_message(). Make sure this brings no bugs
        // if (m_submessage_starts == nullptr) { find_submessages(); }
        if (m_submessage_starts[submessage_index + 1] - m_submessage_starts[submessage_index] > 1) {
            submessage_start = m_submessage_starts[submessage_index] + 1;
        } else {
            submessage_start = m_submessage_starts[submessage_index];
        }
    }
    return submessage_start;
}

bool TokenMessage::submessage_is_single_token(int submessage_index) {
    bool is_single {false};

    if ((submessage_index < m_submessage_count) && (submessage_index >= 0)) {
        // TODO: find_submessages() has been moved to set_message(). Make sure this brings no bugs
        // if (m_submessage_starts == nullptr) { find_submessages(); }
        is_single = m_submessage_starts[submessage_index + 1] - m_submessage_starts[submessage_index] == 1;
    }
    return is_single;
}

int TokenMessage::set_message(const Token *message) {
    int error_location {ADJUDICATOR_NO_ERROR};
    int bracket_count {0};
    int token_ctr {0};

    // Delete any old message
    // FIXME - Don't use delete[]
    if (m_message != nullptr) {
        delete[] m_message;
        m_message = nullptr;
    }

    // FIXME - Don't use delete[]
    if (m_submessage_starts != nullptr) {
        delete[] m_submessage_starts;
        m_submessage_starts = nullptr;
    }

    // Run through the message, counting submessages, and checking the brackets
    m_message_length = NO_MESSAGE;
    m_submessage_count = 0;

    while (message[token_ctr] != TOKEN_END_OF_MESSAGE) {
        if (bracket_count == 0) { m_submessage_count++; }
        if (message[token_ctr] == TOKEN_OPEN_BRACKET) { bracket_count++; }
        if (message[token_ctr] == TOKEN_CLOSE_BRACKET) {
            bracket_count--;
            if (bracket_count < 0) { return token_ctr; }
        }
        token_ctr++;
    }

    // Brackets not matched. Don't accept message
    if (bracket_count != 0) {
        m_submessage_count = NO_MESSAGE;
        return token_ctr;
    }

    // Copy the message in
    // FIXME don't use new - Use smart pointers
    m_message = new Token[token_ctr + 1];
    memcpy(m_message, message, token_ctr * sizeof(Token));
    m_message[token_ctr] = TOKEN_END_OF_MESSAGE;
    m_message_length = token_ctr;

    return error_location;
}

int TokenMessage::set_message(const Token *message, int message_length) {
    int error_location {ADJUDICATOR_NO_ERROR};
    int bracket_count {0};
    int token_ctr {0};

    // Delete any old message
    // FIXME - Don't use delete[]
    if (m_message != nullptr) {
        delete[] m_message;
        m_message = nullptr;
    }

    // FIXME - Don't use delete[]
    if (m_submessage_starts != nullptr) {
        delete[] m_submessage_starts;
        m_submessage_starts = nullptr;
    }

    // Run through the message, counting submessages, and checking the brackets
    m_message_length = NO_MESSAGE;
    m_submessage_count = 0;

    for (token_ctr = 0; token_ctr < message_length; token_ctr++) {
        if (bracket_count == 0) { m_submessage_count++; }
        if (message[token_ctr] == TOKEN_OPEN_BRACKET) { bracket_count++; }
        if (message[token_ctr] == TOKEN_CLOSE_BRACKET) {
            bracket_count--;
            if (bracket_count < 0) { return token_ctr; }
        }
    }

    // Brackets not matched. Don't accept message
    if (bracket_count != 0) {
        m_submessage_count = NO_MESSAGE;
        return token_ctr;
    }

    // Copy the message in
    // FIXME don't use new - Use smart pointers
    m_message = new Token[message_length + 1];
    memcpy(m_message, message, message_length * sizeof(Token));
    m_message[message_length] = TOKEN_END_OF_MESSAGE;
    m_message_length = message_length;

    find_submessages();

    return error_location;
}

int TokenMessage::set_message_from_text(const std::string &text) {
    int token_value {0};
    int error_location {ADJUDICATOR_NO_ERROR};
    int bracket_count {0};
    int char_ctr {0};
    int token_ctr {0};
    char token_text[5] {};
    std::string token_string;
    Token *token_message {nullptr};
    TEXT_TO_TOKEN_MAP *text_to_token_map = &(TokenTextMap::instance()->m_text_to_token_map);

    // FIXME - Use smart pointers instead
    token_message = new Token[text.length()];

    while (char_ctr < text.length()) {
        if (text[char_ctr] == ' ') {
            char_ctr++;                                             // Skip over it

        } else if (text[char_ctr] == '(') {
            token_message[token_ctr] = TOKEN_OPEN_BRACKET;
            bracket_count++;
            token_ctr++;
            char_ctr++;

        } else if (text[char_ctr] == ')') {
            token_message[token_ctr] = TOKEN_CLOSE_BRACKET;
            bracket_count--;
            if (bracket_count < 0) { return char_ctr; }
            token_ctr++;
            char_ctr++;

        } else if (text[char_ctr] == '\'') {
            char_ctr++;

            // Double-apostrophe. Insert a single one into the text
            if ((token_ctr > 0) && (token_message[token_ctr - 1].get_category() == CATEGORY_ASCII)) {
                token_message[token_ctr] = Token(CATEGORY_ASCII, '\'');
                token_ctr++;
            }

            while ((char_ctr < text.length()) && (text[char_ctr] != '\'')) {
                token_message[token_ctr] = Token(CATEGORY_ASCII, text[char_ctr]);
                token_ctr++;
                char_ctr++;
            }

            // Unmatched quote
            if (text[char_ctr] != '\'') { return char_ctr; }
            char_ctr++;                                     // Move to the next character

        } else if (isalpha(text[char_ctr]) != 0) {
            token_text[0] = toupper(text[char_ctr]);
            token_text[1] = toupper(text[char_ctr + 1]);
            token_text[2] = toupper(text[char_ctr + 2]);
            token_text[3] = '\0';
            token_string = token_text;

            auto token_itr = text_to_token_map->find(token_string);
            if (token_itr == text_to_token_map->end()) { return char_ctr; }     // Undefined token

            token_message[token_ctr] = token_itr->second;
            token_ctr++;
            char_ctr = char_ctr + 3;

        } else if (((isdigit(text[char_ctr])) != 0) || (text[char_ctr] == '-')) {
            if (text[char_ctr] == '-') { char_ctr++; }
            token_value = 0;

            while (isdigit(text[char_ctr]) != 0) {
                token_value = token_value * 10 + (text[char_ctr] - '0');
                char_ctr++;
            }
            token_message[token_ctr] = (LANGUAGE_TOKEN) (token_value & ~NUMBER_MASK);
            token_ctr++;

        // Illegal character.
        } else { return char_ctr; }
    }

    // We should be back to 0 brackets
    if (bracket_count != 0) { return text.length(); }

    error_location = set_message(token_message, token_ctr);

    // This should never occur - all errors should have been picked up already.
    // Set the error location to the end of the string
    if (error_location != ADJUDICATOR_NO_ERROR) { return text.length(); }

    // FIXME - Use smart pointers
    delete[] token_message;
    return error_location;
}

void TokenMessage::set_message_as_ascii(const std::string &text) {
    Token *token_message {nullptr};

    // FIXME - Use smart pointers
    token_message = new Token[text.length()];

    for (int token_ctr = 0; (unsigned int) (token_ctr) < text.length(); token_ctr++) {
        token_message[token_ctr] = Token(CATEGORY_ASCII, text[token_ctr]);
    }
    set_message(token_message, text.length());

    // FIXME - Use smart pointers
    delete[] token_message;
}

std::string TokenMessage::get_message_as_text() const {
    bool is_ascii_text {false};
    std::ostringstream message_as_text;
    TOKEN_TO_TEXT_MAP *token_to_text_map = &(TokenTextMap::instance()->m_token_to_text_map);

    for (int token_ctr = 0; token_ctr < m_message_length; token_ctr++) {
        if (is_ascii_text && (m_message[token_ctr].get_category() != CATEGORY_ASCII)) {
            message_as_text << "' ";
            is_ascii_text = false;
        }

        if (!is_ascii_text && (m_message[token_ctr].get_category() == CATEGORY_ASCII)) {
            message_as_text << "'";
            is_ascii_text = true;
        }

        if (is_ascii_text) {
            message_as_text << static_cast<char>(m_message[token_ctr].get_subtoken());
        } else if (m_message[token_ctr].is_number()) {
            message_as_text << m_message[token_ctr].get_number() << " ";
        } else {
            auto token_itr = token_to_text_map->find(m_message[token_ctr].get_token());
            message_as_text << ((token_itr == token_to_text_map->end()) ? "??? " : token_itr->second + " ");
        }
    }

    if (is_ascii_text) { message_as_text << "' "; }

    return message_as_text.str();
}

TokenMessage TokenMessage::enclose() const {
    TokenMessage new_message {};

    if (m_message == nullptr) {
        // FIXME - Use smart pointers instead
        // Create a new buffer
        new_message.m_message = new Token[3];

        // Add the brackets and terminator
        new_message.m_message[0] = TOKEN_OPEN_BRACKET;
        new_message.m_message[1] = TOKEN_CLOSE_BRACKET;
        new_message.m_message[2] = TOKEN_END_OF_MESSAGE;

        // Set the length
        new_message.m_message_length = 2;

        // Update the submessages
        new_message.m_submessage_count = 1;
        return new_message;

    }

    // Create a new buffer
    // FIXME - Use smart pointers instead
    new_message.m_message = new Token[m_message_length + 3];

    // Copy the message
    memcpy(&(new_message.m_message[1]), m_message, m_message_length * sizeof(Token));

    // Add the brackets and terminator
    new_message.m_message[0] = TOKEN_OPEN_BRACKET;
    new_message.m_message[m_message_length + 1] = TOKEN_CLOSE_BRACKET;
    new_message.m_message[m_message_length + 2] = TOKEN_END_OF_MESSAGE;

    // Set the length
    new_message.m_message_length = m_message_length + 2;

    // Update the submessages
    new_message.m_submessage_count = 1;
    return new_message;
}

void TokenMessage::enclose_this() {
    Token *new_message {nullptr};

    if (m_message == nullptr) {
        // Create a new buffer
        // FIXME - Use smart pointers instead
        new_message = new Token[3];

        // Add the brackets and terminator
        new_message[0] = TOKEN_OPEN_BRACKET;
        new_message[1] = TOKEN_CLOSE_BRACKET;
        new_message[2] = TOKEN_END_OF_MESSAGE;

        // Put the new message in its place
        m_message = new_message;
        m_message_length = 2;
        m_submessage_count = 1;

        if (m_submessage_starts != nullptr) {
            m_submessage_starts[0] = 0;
            m_submessage_starts[1] = 2;
        }
        return;
    }

    // Create a new buffer
    // FIXME - Use smart pointers instead
    new_message = new Token[m_message_length + 3];

    // Copy the message
    memcpy(&(new_message[1]), m_message, m_message_length * sizeof(Token));

    // Add the brackets and terminator
    new_message[0] = TOKEN_OPEN_BRACKET;
    new_message[m_message_length + 1] = TOKEN_CLOSE_BRACKET;
    new_message[m_message_length + 2] = TOKEN_END_OF_MESSAGE;

    // Delete the old message
    // FIXME - Use smart pointers instead
    delete[] m_message;

    // Put the new message in its place
    m_message = new_message;
    m_message_length = m_message_length + 2;
    m_submessage_count = 1;

    if (m_submessage_starts != nullptr) {
        m_submessage_starts[0] = 0;
        m_submessage_starts[1] = m_message_length;
    }

}

// Appends a message
TokenMessage TokenMessage::operator&(const TokenMessage &other_message) {
    TokenMessage new_message {};                        // The new message

    if (m_message == nullptr) { return other_message.enclose(); }
    if (other_message.m_message == nullptr) { return *this + other_message.enclose(); }

    // Create a new buffer
    // FIXME - Use smart pointers instead
    new_message.m_message = new Token[m_message_length + other_message.m_message_length + 3];

    // Copy the message
    memcpy(new_message.m_message, m_message, m_message_length * sizeof(Token));

    // Copy the other message in
    memcpy(&(new_message.m_message[m_message_length + 1]),
           other_message.m_message,
           other_message.m_message_length * sizeof(Token));

    // Add the brackets and terminator
    new_message.m_message[m_message_length] = TOKEN_OPEN_BRACKET;
    new_message.m_message[m_message_length + other_message.m_message_length + 1] = TOKEN_CLOSE_BRACKET;
    new_message.m_message[m_message_length + other_message.m_message_length + 2] = TOKEN_END_OF_MESSAGE;

    // Update the length
    new_message.m_message_length = m_message_length + other_message.m_message_length + 2;

    // Update the submessages
    new_message.m_submessage_count = m_submessage_count + 1;

    return new_message;
}

// Message to append as a submessage
TokenMessage TokenMessage::operator+(const TokenMessage &other_message) {
    TokenMessage new_message {};                        // The new message

    if (m_message == nullptr) { return other_message; }
    if (other_message.m_message == nullptr) { return *this; }

    // Create a new buffer
    // FIXME - Use smart pointers instead
    new_message.m_message = new Token[m_message_length + other_message.m_message_length + 1];

    // Copy the message
    memcpy(new_message.m_message, m_message, m_message_length * sizeof(Token));

    // Copy the other message in
    memcpy(&(new_message.m_message[m_message_length]),
           other_message.m_message,
           other_message.m_message_length * sizeof(Token));

    // Add the brackets and terminator
    new_message.m_message[m_message_length + other_message.m_message_length] = TOKEN_END_OF_MESSAGE;

    // Update the length
    new_message.m_message_length = m_message_length + other_message.m_message_length;

    // Update the submessages
    new_message.m_submessage_count = m_submessage_count + other_message.m_submessage_count;

    return new_message;
}

TokenMessage TokenMessage::operator&(const Token &token) {
    TokenMessage token_message(token);
    return operator&(token_message);
}

TokenMessage TokenMessage::operator+(const Token &token) {
    TokenMessage token_message(token);
    return operator+(token_message);
}

void TokenMessage::find_submessages() {
    int bracket_count {0};
    int submessage_count {0};

    // FIXME - Use smart pointers instead
    m_submessage_starts = new int[m_submessage_count + 1];

    for (int token_ctr = 0; token_ctr < m_message_length; token_ctr++) {
        if (bracket_count == 0) {
            m_submessage_starts[submessage_count] = token_ctr;
            submessage_count++;
        }
        if (m_message[token_ctr] == TOKEN_OPEN_BRACKET) { bracket_count++; }
        if (m_message[token_ctr] == TOKEN_CLOSE_BRACKET) { bracket_count--; }
    }
    m_submessage_starts[m_submessage_count] = m_message_length;
}

bool TokenMessage::operator<(const TokenMessage &other) const {
    int token_ctr {0};
    int tokens_to_compare = (m_message_length < other.m_message_length) ? m_message_length : other.m_message_length;

    if (other.m_message == nullptr) { return false; }                               // less_than = false
    if (m_message == nullptr) { return true; }                                      // less_than = true

    while (token_ctr < tokens_to_compare) {
        if (m_message[token_ctr] < other.m_message[token_ctr]) { return true; }     // less_than = true
        if (other.m_message[token_ctr] < m_message[token_ctr]) { return false; }    // less_than = false
        token_ctr++;
    }

    return tokens_to_compare < other.m_message_length;
}

bool TokenMessage::operator==(const TokenMessage &other) const {
    int token_ctr {0};

    if ((other.m_message == nullptr) || (m_message == nullptr) || (m_message_length != other.m_message_length)) {
        return false;
    }
    while (token_ctr < m_message_length) {
        if (m_message[token_ctr] != other.m_message[token_ctr]) { return false; }
        token_ctr++;
    }
    return true;
}

bool TokenMessage::operator!=(const TokenMessage &other) const {
    return !(operator==(other));
}

TokenMessage Token::operator+(const Token &token) const {
    TokenMessage message(*this);
    return message + token;
}

TokenMessage Token::operator&(const Token &token) const {
    TokenMessage message(*this);
    return message & token;
}

TokenMessage Token::operator+(const TokenMessage &token_message) const {
    TokenMessage message(*this);
    return message + token_message;
}

TokenMessage Token::operator&(const TokenMessage &token_message) const {
    TokenMessage message(*this);
    return message & token_message;
}
