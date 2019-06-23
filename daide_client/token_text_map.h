/**
 * Diplomacy AI Server - Part of the DAIDE project.
 *
 * TokenTextMap Class Header.
 *
 * (C) David Norman 2002 david@ellought.demon.co.uk
 *
 * This software may be reused for non-commercial purposes without charge, and
 * without notifying the author. Use of any part of this software for commercial 
 * purposes without permission from the Author is prohibited.
 *
 * Release 8~2
 **/

#ifndef _DAIDE_TOKEN_TEXT_MAP
#define _DAIDE_TOKEN_TEXT_MAP

#include "tokens.h"

typedef map<Token, string> TOKEN_TO_TEXT_MAP;
typedef map<string, Token> TEXT_TO_TOKEN_MAP;

class TokenTextMap {
public:
    static TokenTextMap *instance();

    TOKEN_TO_TEXT_MAP m_token_to_text_map;
    TEXT_TO_TOKEN_MAP m_text_to_token_map;

    void clear_category(BYTE category);

    void clear_power_and_province_categories();

    bool add_token(const Token &token, const string token_string);

private:
    TokenTextMap();
};

#endif // _DAIDE_TOKEN_TEXT_MAP
