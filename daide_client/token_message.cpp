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
 * Release 8~2
 **/

#include "stdafx.h"
#include <sstream>
#include "token_message.h"
#include "token_text_map.h"

TokenMessage::TokenMessage() {
    // Set the message to blank
    m_message = NULL;
    m_message_length = NO_MESSAGE;
    m_submessage_starts = NULL;
    m_submessage_count = NO_MESSAGE;
}

TokenMessage::TokenMessage(const Token *message)    // The message to set it to
{
    m_message = NULL;
    m_submessage_starts = NULL;

    // Set the message as specified
    set_message(message);
}

TokenMessage::TokenMessage(const Token *message,    // The message to set it to
                           int message_length)        // The length of the message
{
    m_message = NULL;
    m_submessage_starts = NULL;

    // Set the message as specified
    set_message(message, message_length);
}

TokenMessage::TokenMessage(const Token &token)    // The token to set it to
{
    m_message = NULL;
    m_submessage_starts = NULL;

    // Set the message as specified
    set_message(&token, 1);
}

TokenMessage::TokenMessage(const TokenMessage &message_to_copy)    // The message to copy
{
    m_message = NULL;
    m_submessage_starts = NULL;

    if (message_to_copy.m_message != NULL) {
        set_message(message_to_copy.m_message, message_to_copy.m_message_length);
    } else {
        // Set the message to blank
        m_message = NULL;
        m_message_length = NO_MESSAGE;
        m_submessage_starts = NULL;
        m_submessage_count = NO_MESSAGE;
    }
}

TokenMessage::~TokenMessage() {
    if (m_message != NULL) {
        delete[] m_message;
        m_message = NULL;
    }

    if (m_submessage_starts != NULL) {
        delete[] m_submessage_starts;
        m_submessage_starts = NULL;
    }
}

TokenMessage &TokenMessage::operator=(const TokenMessage &message_to_copy)        // The message to copy
{
    // Delete any old message
    if (m_message != NULL) {
        delete[] m_message;
        m_message = NULL;
    }

    if (m_submessage_starts != NULL) {
        delete[] m_submessage_starts;
        m_submessage_starts = NULL;
    }

    m_message_length = NO_MESSAGE;
    m_submessage_count = NO_MESSAGE;

    // Copy the message
    if (message_to_copy.m_message != NULL) {
        set_message(message_to_copy.m_message, message_to_copy.m_message_length);
    }

    return *this;
}

bool TokenMessage::get_message(Token message[],        // Buffer to copy message into
                               int buffer_length)        // Size of buffer available
const {
    bool message_copied = true;            // Whether the message is set

    if (m_message == NULL) {
        message_copied = false;
    }

        // Check if the buffer is big enough
    else if (buffer_length <= m_message_length) {
        message_copied = false;
    } else {
        // Copy the data
        memcpy(message, m_message, (m_message_length + 1) * sizeof(Token));
    }

    return message_copied;
}

int TokenMessage::get_message_length() const {
    return m_message_length;
}

bool TokenMessage::is_single_token() const {
    return (m_message_length == 1);
}

bool TokenMessage::contains_submessages() const {
    return (m_message_length != m_submessage_count);
}

Token TokenMessage::get_token() const {
    return m_message[0];
}

Token TokenMessage::get_token(int index)            // Index of the token to get
const {
    Token token;            // The token to return

    if ((index >= m_message_length) || (index < 0)) {
        token = TOKEN_END_OF_MESSAGE;
    } else {
        token = m_message[index];
    }

    return token;
}

int TokenMessage::get_submessage_count() const {
    int submessage_count;

    if (m_submessage_count == NO_MESSAGE) {
        submessage_count = 0;
    } else {
        submessage_count = m_submessage_count;
    }

    return submessage_count;
}

TokenMessage TokenMessage::get_submessage(int submessage_index)        // Index of the submessage
{
    TokenMessage submessage;            // The submessage to return
    int submessage_length;                // Length of the submessage to copy

    if (m_message != NULL) {
        if (m_submessage_starts == NULL) {
            find_submessages();
        }

        if (submessage_index < m_submessage_count) {
            // Find the length of the submessage
            submessage_length = m_submessage_starts[submessage_index + 1] - m_submessage_starts[submessage_index];

            // If it is one token, then just copy the token
            if (submessage_length == 1) {
                submessage.set_message(&(m_message[m_submessage_starts[submessage_index]]), 1);
            } else {
                // Copy the submessage, but leave off the start and end brackets
                submessage.set_message(&(m_message[m_submessage_starts[submessage_index] + 1]),
                                       submessage_length - 2);
            }
        }
    }

    return submessage;
}

int TokenMessage::get_submessage_start(int submessage_index) {
    int submessage_start;

    if ((submessage_index >= m_submessage_count)
        || (submessage_index < 0)) {
        submessage_start = NO_MESSAGE;
    } else {
        if (m_submessage_starts == NULL) {
            find_submessages();
        }

        if (m_submessage_starts[submessage_index + 1] - m_submessage_starts[submessage_index] > 1) {
            submessage_start = m_submessage_starts[submessage_index] + 1;
        } else {
            submessage_start = m_submessage_starts[submessage_index];
        }
    }

    return submessage_start;
}

bool TokenMessage::submessage_is_single_token(int submessage_index) {
    bool is_single;

    if ((submessage_index >= m_submessage_count)
        || (submessage_index < 0)) {
        is_single = false;
    } else {
        if (m_submessage_starts == NULL) {
            find_submessages();
        }

        if (m_submessage_starts[submessage_index + 1] - m_submessage_starts[submessage_index] == 1) {
            is_single = true;
        } else {
            is_single = false;
        }
    }

    return is_single;
}

int TokenMessage::set_message(const Token *message)        // Message to set it to
{
    int error_location = ADJUDICATOR_NO_ERROR;
    int bracket_count = 0;
    int token_counter = 0;

    // Delete any old message
    if (m_message != NULL) {
        delete[] m_message;
        m_message = NULL;
    }

    if (m_submessage_starts != NULL) {
        delete[] m_submessage_starts;
        m_submessage_starts = NULL;
    }

    m_message_length = NO_MESSAGE;

    // Run through the message, counting submessages, and checking the brackets
    m_submessage_count = 0;

    while ((message[token_counter] != TOKEN_END_OF_MESSAGE)
           && (error_location == ADJUDICATOR_NO_ERROR)) {
        if (bracket_count == 0) {
            m_submessage_count++;
        }

        if (message[token_counter] == TOKEN_OPEN_BRACKET) {
            bracket_count++;
        }

        if (message[token_counter] == TOKEN_CLOSE_BRACKET) {
            bracket_count--;

            if (bracket_count < 0) {
                error_location = token_counter;
            }
        }

        token_counter++;
    }

    if (bracket_count != 0) {
        // Brackets not matched. Don't accept message
        error_location = token_counter;

        m_submessage_count = NO_MESSAGE;
    } else {
        // Copy the message in
        m_message = new Token[token_counter + 1];

        memcpy(m_message, message, token_counter * sizeof(Token));

        m_message[token_counter] = TOKEN_END_OF_MESSAGE;

        m_message_length = token_counter;
    }

    return error_location;
}

int TokenMessage::set_message(const Token *message,        // Message to set it to
                              int message_length)            // Length of the message
{
    int error_location = ADJUDICATOR_NO_ERROR;
    int bracket_count = 0;
    int token_counter = 0;

    // Delete any old message
    if (m_message != NULL) {
        delete[] m_message;
        m_message = NULL;
    }

    if (m_submessage_starts != NULL) {
        delete[] m_submessage_starts;
        m_submessage_starts = NULL;
    }

    m_message_length = NO_MESSAGE;

    // Run through the message, counting submessages, and checking the brackets
    m_submessage_count = 0;

    for (token_counter = 0;
         (token_counter < message_length) && (error_location == ADJUDICATOR_NO_ERROR);
         token_counter++) {
        if (bracket_count == 0) {
            m_submessage_count++;
        }

        if (message[token_counter] == TOKEN_OPEN_BRACKET) {
            bracket_count++;
        }

        if (message[token_counter] == TOKEN_CLOSE_BRACKET) {
            bracket_count--;

            if (bracket_count < 0) {
                error_location = token_counter;
            }
        }
    }

    if (bracket_count != 0) {
        // Brackets not matched. Don't accept message
        error_location = token_counter;

        m_submessage_count = NO_MESSAGE;
    } else {
        // Copy the message in
        m_message = new Token[message_length + 1];

        memcpy(m_message, message, message_length * sizeof(Token));

        m_message[message_length] = TOKEN_END_OF_MESSAGE;

        m_message_length = message_length;
    }

    return error_location;
}

int TokenMessage::set_message_from_text(string text) {
    int error_location = ADJUDICATOR_NO_ERROR;
    int char_counter = 0;
    int token_counter = 0;
    string token_string;
    Token *token_message;
    TEXT_TO_TOKEN_MAP *text_to_token_map = &(TokenTextMap::instance()->m_text_to_token_map);
    TEXT_TO_TOKEN_MAP::iterator token_iterator;
    bool is_negative;
    int token_value;
    char token_text[5];
    int bracket_count = 0;

    token_message = new Token[text.length()];

    while ((error_location == ADJUDICATOR_NO_ERROR)
           && ((unsigned int) (char_counter) < text.length())) {
        if (text[char_counter] == ' ') {
            // Skip over it
            char_counter++;
        } else if (text[char_counter] == '(') {
            token_message[token_counter] = TOKEN_OPEN_BRACKET;
            bracket_count++;
            token_counter++;
            char_counter++;
        } else if (text[char_counter] == ')') {
            token_message[token_counter] = TOKEN_CLOSE_BRACKET;
            bracket_count--;

            if (bracket_count < 0) {
                error_location = char_counter;
            }

            token_counter++;
            char_counter++;
        } else if (text[char_counter] == '\'') {
            char_counter++;

            if ((token_counter > 0)
                && (token_message[token_counter - 1].get_category() == CATEGORY_ASCII)) {
                // Double-apostrophe. Insert a single one into the text
                token_message[token_counter] = Token(CATEGORY_ASCII, '\'');
                token_counter++;
            }

            while (((unsigned int) (char_counter) < text.length())
                   && (text[char_counter] != '\'')) {
                token_message[token_counter] = Token(CATEGORY_ASCII, text[char_counter]);
                token_counter++;
                char_counter++;
            }

            if (text[char_counter] != '\'') {
                // Unmatched quote
                error_location = char_counter;
            } else {
                // Move to the next character
                char_counter++;
            }
        } else if (isalpha(text[char_counter])) {

            token_text[0] = toupper(text[char_counter]);
            token_text[1] = toupper(text[char_counter + 1]);
            token_text[2] = toupper(text[char_counter + 2]);
            token_text[3] = '\0';

            token_string = token_text;

            token_iterator = text_to_token_map->find(token_string);

            if (token_iterator == text_to_token_map->end()) {
                // Undefined token
                error_location = char_counter;
            } else {
                token_message[token_counter] = token_iterator->second;
                token_counter++;
                char_counter = char_counter + 3;
            }
        } else if ((isdigit(text[char_counter])) || (text[char_counter] == '-')) {
            if (text[char_counter] == '-') {
                is_negative = true;
                char_counter++;
            } else {
                is_negative = false;
            }

            token_value = 0;

            while (isdigit(text[char_counter])) {
                token_value = token_value * 10 + (text[char_counter] - '0');
                char_counter++;
            }

            token_message[token_counter] = (LANGUAGE_TOKEN) (token_value & ~NUMBER_MASK);
            token_counter++;
        } else {
            // Illegal character.
            error_location = char_counter;
        }
    }

    if (error_location == ADJUDICATOR_NO_ERROR) {
        // We should be back to 0 brackets
        if (bracket_count != 0) {
            error_location = text.length();
        } else {
            error_location = set_message(token_message, token_counter);

            if (error_location != ADJUDICATOR_NO_ERROR) {
                // This should never occur - all errors should have been picked up already.
                // Set the error location to the end of the string
                error_location = text.length();
            }
        }
    }

    delete[] token_message;

    return error_location;
}

void TokenMessage::set_message_as_ascii(const string text) {
    int token_counter;
    Token *token_message;

    token_message = new Token[text.length()];

    for (token_counter = 0; (unsigned int) (token_counter) < text.length(); token_counter++) {
        token_message[token_counter] = Token(CATEGORY_ASCII, text[token_counter]);
    }

    set_message(token_message, text.length());

    delete[] token_message;
}

string TokenMessage::get_message_as_text() const {
    ostringstream message_as_text;
    int token_counter;
    bool is_ascii_text = false;
    TOKEN_TO_TEXT_MAP *token_to_text_map = &(TokenTextMap::instance()->m_token_to_text_map);
    TOKEN_TO_TEXT_MAP::iterator token_iterator;

    for (token_counter = 0; token_counter < m_message_length; token_counter++) {
        if ((is_ascii_text)
            && (m_message[token_counter].get_category() != CATEGORY_ASCII)) {
            message_as_text << "' ";
            is_ascii_text = false;
        }

        if ((is_ascii_text == false)
            && (m_message[token_counter].get_category() == CATEGORY_ASCII)) {
            message_as_text << "'";
            is_ascii_text = true;
        }

        if (is_ascii_text) {
            message_as_text << (char) (m_message[token_counter].get_subtoken());
        } else if (m_message[token_counter].is_number()) {
            message_as_text << m_message[token_counter].get_number() << " ";
        } else {
            token_iterator = token_to_text_map->find(m_message[token_counter].get_token());

            if (token_iterator == token_to_text_map->end()) {
                message_as_text << "??? ";
            } else {
                message_as_text << token_iterator->second + " ";
            }
        }
    }

    if (is_ascii_text) {
        message_as_text << "' ";
    }

    return message_as_text.str();
}

TokenMessage TokenMessage::enclose() const {
    TokenMessage new_message;

    if (m_message == NULL) {
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
    } else {
        // Create a new buffer
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
    }

    return new_message;
}

void TokenMessage::enclose_this() {
    Token *new_message;                    // The enclosed message

    if (m_message == NULL) {
        // Create a new buffer
        new_message = new Token[3];

        // Add the brackets and terminator
        new_message[0] = TOKEN_OPEN_BRACKET;
        new_message[1] = TOKEN_CLOSE_BRACKET;
        new_message[2] = TOKEN_END_OF_MESSAGE;

        // Put the new message in its place
        m_message = new_message;

        // Update the length
        m_message_length = 2;

        // Update the submessages
        m_submessage_count = 1;

        if (m_submessage_starts != NULL) {
            m_submessage_starts[0] = 0;
            m_submessage_starts[1] = 2;
        }
    } else {
        // Create a new buffer
        new_message = new Token[m_message_length + 3];

        // Copy the message
        memcpy(&(new_message[1]), m_message, m_message_length * sizeof(Token));

        // Add the brackets and terminator
        new_message[0] = TOKEN_OPEN_BRACKET;
        new_message[m_message_length + 1] = TOKEN_CLOSE_BRACKET;
        new_message[m_message_length + 2] = TOKEN_END_OF_MESSAGE;

        // Delete the old message
        delete[] m_message;

        // Put the new message in its place
        m_message = new_message;

        // Update the length
        m_message_length = m_message_length + 2;

        // Update the submessages
        m_submessage_count = 1;

        if (m_submessage_starts != NULL) {
            m_submessage_starts[0] = 0;
            m_submessage_starts[1] = m_message_length;
        }
    }
}

TokenMessage TokenMessage::operator&(const TokenMessage &other_message)        // Message to append as a submessage
{
    TokenMessage new_message;            // The new message

    if (m_message == NULL) {
        new_message = other_message.enclose();
    } else if (other_message.m_message == NULL) {
        new_message = *this + other_message.enclose();
    } else {
        // Create a new buffer
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
    }

    return new_message;
}

TokenMessage TokenMessage::operator+(const TokenMessage &other_message)        // Message to append as a submessage
{
    TokenMessage new_message;            // The new message

    if (m_message == NULL) {
        new_message = other_message;
    } else if (other_message.m_message == NULL) {
        new_message = *this;
    } else {
        // Create a new buffer
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
    }

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
    int bracket_count = 0;
    int token_counter;
    int submessage_count = 0;

    m_submessage_starts = new int[m_submessage_count + 1];

    for (token_counter = 0; token_counter < m_message_length; token_counter++) {
        if (bracket_count == 0) {
            m_submessage_starts[submessage_count] = token_counter;

            submessage_count++;
        }

        if (m_message[token_counter] == TOKEN_OPEN_BRACKET) {
            bracket_count++;
        }

        if (m_message[token_counter] == TOKEN_CLOSE_BRACKET) {
            bracket_count--;
        }
    }

    m_submessage_starts[m_submessage_count] = m_message_length;
}

bool TokenMessage::operator<(const TokenMessage &other) const {
    bool is_less_than;
    bool difference_found = false;
    int token_counter = 0;
    int tokens_to_compare = (m_message_length < other.m_message_length) ? m_message_length : other.m_message_length;

    if (other.m_message == NULL) {
        is_less_than = false;
        difference_found = true;
    } else if (m_message == NULL) {
        is_less_than = true;
        difference_found = true;
    }

    while ((difference_found == false)
           && (token_counter < tokens_to_compare)) {
        if (m_message[token_counter] < other.m_message[token_counter]) {
            is_less_than = true;
            difference_found = true;
        } else if (other.m_message[token_counter] < m_message[token_counter]) {
            is_less_than = false;
            difference_found = true;
        }

        token_counter++;
    }

    if (difference_found == false) {
        if (tokens_to_compare < other.m_message_length) {
            is_less_than = true;
        } else {
            is_less_than = false;
        }
    }

    return is_less_than;
}

bool TokenMessage::operator==(const TokenMessage &other) const {
    bool difference_found = false;
    int token_counter = 0;

    if ((other.m_message == NULL)
        || (m_message == NULL)
        || (m_message_length != other.m_message_length)) {
        difference_found = true;
    }

    while ((difference_found == false)
           && (token_counter < m_message_length)) {
        if (m_message[token_counter] != other.m_message[token_counter]) {
            difference_found = true;
        }

        token_counter++;
    }

    return !(difference_found);
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

