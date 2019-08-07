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
 * Release 8~3
 **/

#ifndef _DAIDE_CLIENT_DAIDE_CLIENT_TOKEN_TEXT_MAP_H
#define _DAIDE_CLIENT_DAIDE_CLIENT_TOKEN_TEXT_MAP_H

#include "daide_client/tokens.h"

namespace DAIDE {

using TOKEN_TO_TEXT_MAP = std::map<Token, std::string>;
using TEXT_TO_TOKEN_MAP = std::map<std::string, Token>;

class TokenTextMap {
public:
    static TokenTextMap *instance();

    TOKEN_TO_TEXT_MAP m_token_to_text_map;
    TEXT_TO_TOKEN_MAP m_text_to_token_map;

    void clear_category(BYTE category);

    void clear_power_and_province_categories();

    bool add_token(const Token &token, const std::string &token_string);

private:
    TokenTextMap();
};

} // namespace DAIDE

#endif // _DAIDE_CLIENT_DAIDE_CLIENT_TOKEN_TEXT_MAP_H
