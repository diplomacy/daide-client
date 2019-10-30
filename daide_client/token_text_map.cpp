/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * TokenTextMap Class. Provides conversion between 16 bit encoding and text.
 *
 * (C) David Norman 2002 david@ellought.demon.co.uk
 *
 * This software may be reused for non-commercial purposes without charge, and
 * without notifying the author. Use of any part of this software for commercial
 * purposes without permission from the Author is prohibited.
 *
 * Release 8~3
 **/

#include "token_text_map.h"

using DAIDE::TokenTextMap;

TokenTextMap *TokenTextMap::instance() {
    static TokenTextMap the_instance;
    return &the_instance;
}

void TokenTextMap::clear_category(BYTE category) {
    TOKEN_TO_TEXT_MAP::iterator token_to_text_itr;
    LANGUAGE_TOKEN category_value = (LANGUAGE_TOKEN) (category) << 8;
    LANGUAGE_TOKEN next_category_value = (LANGUAGE_TOKEN) (category + 1) << 8;
    std::string token_string;

    // Find the first element in the specified category
    token_to_text_itr = m_token_to_text_map.lower_bound(category_value);

    // While the end of the category is not found
    while ((token_to_text_itr != m_token_to_text_map.end()) && (token_to_text_itr->first < next_category_value)) {

        // Get the string for the current token
        token_string = token_to_text_itr->second;

        // Erase the entry from the text to token map
        m_text_to_token_map.erase(token_string);

        // Erase the entry from the token to text map
        token_to_text_itr = m_token_to_text_map.erase(token_to_text_itr);
    }
}

void TokenTextMap::clear_power_and_province_categories() {
    clear_category(CATEGORY_POWER);
    for (BYTE category_ctr = CATEGORY_PROVINCE_MIN; category_ctr <= CATEGORY_PROVINCE_MAX; category_ctr++) {
        clear_category(category_ctr);
    }
}

bool TokenTextMap::add_token(const Token &token, const std::string &token_string) {
    bool added_ok {true};

    // Already added, not readding
    if ((m_token_to_text_map.find(token) != m_token_to_text_map.end())
            || (m_text_to_token_map.find(token_string) != m_text_to_token_map.end())) {

        added_ok = false;

    // Storing in map
    } else {
        m_token_to_text_map[token] = token_string;
        m_text_to_token_map[token_string] = token;
    }
    return added_ok;
}

TokenTextMap::TokenTextMap() {
    add_token(TOKEN_OPEN_BRACKET, "(");
    add_token(TOKEN_CLOSE_BRACKET, ")");
    add_token(TOKEN_POWER_AUS, "AUS");
    add_token(TOKEN_POWER_ENG, "ENG");
    add_token(TOKEN_POWER_FRA, "FRA");
    add_token(TOKEN_POWER_GER, "GER");
    add_token(TOKEN_POWER_ITA, "ITA");
    add_token(TOKEN_POWER_RUS, "RUS");
    add_token(TOKEN_POWER_TUR, "TUR");
    add_token(TOKEN_UNIT_AMY, "AMY");
    add_token(TOKEN_UNIT_FLT, "FLT");
    add_token(TOKEN_ORDER_CTO, "CTO");
    add_token(TOKEN_ORDER_CVY, "CVY");
    add_token(TOKEN_ORDER_HLD, "HLD");
    add_token(TOKEN_ORDER_MTO, "MTO");
    add_token(TOKEN_ORDER_SUP, "SUP");
    add_token(TOKEN_ORDER_VIA, "VIA");
    add_token(TOKEN_ORDER_DSB, "DSB");
    add_token(TOKEN_ORDER_RTO, "RTO");
    add_token(TOKEN_ORDER_BLD, "BLD");
    add_token(TOKEN_ORDER_REM, "REM");
    add_token(TOKEN_ORDER_WVE, "WVE");
    add_token(TOKEN_ORDER_NOTE_MBV, "MBV");
    add_token(TOKEN_ORDER_NOTE_BPR, "BPR");
    add_token(TOKEN_ORDER_NOTE_CST, "CST");
    add_token(TOKEN_ORDER_NOTE_ESC, "ESC");
    add_token(TOKEN_ORDER_NOTE_FAR, "FAR");
    add_token(TOKEN_ORDER_NOTE_HSC, "HSC");
    add_token(TOKEN_ORDER_NOTE_NAS, "NAS");
    add_token(TOKEN_ORDER_NOTE_NMB, "NMB");
    add_token(TOKEN_ORDER_NOTE_NMR, "NMR");
    add_token(TOKEN_ORDER_NOTE_NRN, "NRN");
    add_token(TOKEN_ORDER_NOTE_NRS, "NRS");
    add_token(TOKEN_ORDER_NOTE_NSC, "NSC");
    add_token(TOKEN_ORDER_NOTE_NSF, "NSF");
    add_token(TOKEN_ORDER_NOTE_NSP, "NSP");
    add_token(TOKEN_ORDER_NOTE_NSU, "NSU");
    add_token(TOKEN_ORDER_NOTE_NYU, "NYU");
    add_token(TOKEN_ORDER_NOTE_YSC, "YSC");
    add_token(TOKEN_RESULT_SUC, "SUC");
    add_token(TOKEN_RESULT_BNC, "BNC");
    add_token(TOKEN_RESULT_CUT, "CUT");
    add_token(TOKEN_RESULT_DSR, "DSR");
    add_token(TOKEN_RESULT_FLD, "FLD");
    add_token(TOKEN_RESULT_NSO, "NSO");
    add_token(TOKEN_RESULT_RET, "RET");
    add_token(TOKEN_COAST_NCS, "NCS");
    add_token(TOKEN_COAST_ECS, "ECS");
    add_token(TOKEN_COAST_SCS, "SCS");
    add_token(TOKEN_COAST_WCS, "WCS");
    add_token(TOKEN_SEASON_SPR, "SPR");
    add_token(TOKEN_SEASON_SUM, "SUM");
    add_token(TOKEN_SEASON_FAL, "FAL");
    add_token(TOKEN_SEASON_AUT, "AUT");
    add_token(TOKEN_SEASON_WIN, "WIN");
    add_token(TOKEN_COMMAND_CCD, "CCD");
    add_token(TOKEN_COMMAND_DRW, "DRW");
    add_token(TOKEN_COMMAND_FRM, "FRM");
    add_token(TOKEN_COMMAND_GOF, "GOF");
    add_token(TOKEN_COMMAND_HLO, "HLO");
    add_token(TOKEN_COMMAND_HST, "HST");
    add_token(TOKEN_COMMAND_HUH, "HUH");
    add_token(TOKEN_COMMAND_IAM, "IAM");
    add_token(TOKEN_COMMAND_LOD, "LOD");
    add_token(TOKEN_COMMAND_MIS, "MIS");
    add_token(TOKEN_COMMAND_NME, "NME");
    add_token(TOKEN_COMMAND_NOT, "NOT");
    add_token(TOKEN_COMMAND_NOW, "NOW");
    add_token(TOKEN_COMMAND_OBS, "OBS");
    add_token(TOKEN_COMMAND_OFF, "OFF");
    add_token(TOKEN_COMMAND_ORD, "ORD");
    add_token(TOKEN_COMMAND_PRN, "PRN");
    add_token(TOKEN_COMMAND_REJ, "REJ");
    add_token(TOKEN_COMMAND_SCO, "SCO");
    add_token(TOKEN_COMMAND_SLO, "SLO");
    add_token(TOKEN_COMMAND_SND, "SND");
    add_token(TOKEN_COMMAND_SUB, "SUB");
    add_token(TOKEN_COMMAND_SVE, "SVE");
    add_token(TOKEN_COMMAND_THX, "THX");
    add_token(TOKEN_COMMAND_TME, "TME");
    add_token(TOKEN_COMMAND_YES, "YES");
    add_token(TOKEN_PARAMETER_AOA, "AOA");
    add_token(TOKEN_PARAMETER_ERR, "ERR");
    add_token(TOKEN_PARAMETER_LVL, "LVL");
    add_token(TOKEN_PARAMETER_MRT, "MRT");
    add_token(TOKEN_PARAMETER_MTL, "MTL");
    add_token(TOKEN_PARAMETER_NPB, "NPB");
    add_token(TOKEN_PARAMETER_NPR, "NPR");
    add_token(TOKEN_PARAMETER_PDA, "PDA");
    add_token(TOKEN_PARAMETER_PTL, "PTL");
    add_token(TOKEN_PARAMETER_RTL, "RTL");
    add_token(TOKEN_PRESS_ALY, "ALY");
    add_token(TOKEN_PRESS_AND, "AND");
    add_token(TOKEN_PRESS_BWX, "BWX");
    add_token(TOKEN_PRESS_DMZ, "DMZ");
    add_token(TOKEN_PRESS_ELS, "ELS");
    add_token(TOKEN_PRESS_EXP, "EXP");
    add_token(TOKEN_PRESS_FOR, "FOR");
    add_token(TOKEN_PRESS_HOW, "HOW");
    add_token(TOKEN_PRESS_IDK, "IDK");
    add_token(TOKEN_PRESS_IFF, "IFF");
    add_token(TOKEN_PRESS_INS, "INS");
    add_token(TOKEN_PRESS_OCC, "OCC");
    add_token(TOKEN_PRESS_ORR, "ORR");
    add_token(TOKEN_PRESS_PCE, "PCE");
    add_token(TOKEN_PRESS_PRP, "PRP");
    add_token(TOKEN_PRESS_QRY, "QRY");
    add_token(TOKEN_PRESS_SCD, "SCD");
    add_token(TOKEN_PRESS_SRY, "SRY");
    add_token(TOKEN_PRESS_SUG, "SUG");
    add_token(TOKEN_PRESS_THK, "THK");
    add_token(TOKEN_PRESS_THN, "THN");
    add_token(TOKEN_PRESS_TRY, "TRY");
    add_token(TOKEN_PRESS_VSS, "VSS");
    add_token(TOKEN_PRESS_WHT, "WHT");
    add_token(TOKEN_PRESS_XDO, "XDO");
    add_token(TOKEN_PRESS_XOY, "XOY");
    add_token(TOKEN_PROVINCE_BOH, "BOH");
    add_token(TOKEN_PROVINCE_BUR, "BUR");
    add_token(TOKEN_PROVINCE_GAL, "GAL");
    add_token(TOKEN_PROVINCE_RUH, "RUH");
    add_token(TOKEN_PROVINCE_SIL, "SIL");
    add_token(TOKEN_PROVINCE_TYR, "TYR");
    add_token(TOKEN_PROVINCE_UKR, "UKR");
    add_token(TOKEN_PROVINCE_BUD, "BUD");
    add_token(TOKEN_PROVINCE_MOS, "MOS");
    add_token(TOKEN_PROVINCE_MUN, "MUN");
    add_token(TOKEN_PROVINCE_PAR, "PAR");
    add_token(TOKEN_PROVINCE_SER, "SER");
    add_token(TOKEN_PROVINCE_VIE, "VIE");
    add_token(TOKEN_PROVINCE_WAR, "WAR");
    add_token(TOKEN_PROVINCE_ADR, "ADR");
    add_token(TOKEN_PROVINCE_AEG, "AEG");
    add_token(TOKEN_PROVINCE_BAL, "BAL");
    add_token(TOKEN_PROVINCE_BAR, "BAR");
    add_token(TOKEN_PROVINCE_BLA, "BLA");
    add_token(TOKEN_PROVINCE_EAS, "EAS");
    add_token(TOKEN_PROVINCE_ECH, "ECH");
    add_token(TOKEN_PROVINCE_GOB, "GOB");
    add_token(TOKEN_PROVINCE_GOL, "GOL");
    add_token(TOKEN_PROVINCE_HEL, "HEL");
    add_token(TOKEN_PROVINCE_ION, "ION");
    add_token(TOKEN_PROVINCE_IRI, "IRI");
    add_token(TOKEN_PROVINCE_MAO, "MAO");
    add_token(TOKEN_PROVINCE_NAO, "NAO");
    add_token(TOKEN_PROVINCE_NTH, "NTH");
    add_token(TOKEN_PROVINCE_NWG, "NWG");
    add_token(TOKEN_PROVINCE_SKA, "SKA");
    add_token(TOKEN_PROVINCE_TYS, "TYS");
    add_token(TOKEN_PROVINCE_WES, "WES");
    add_token(TOKEN_PROVINCE_ALB, "ALB");
    add_token(TOKEN_PROVINCE_APU, "APU");
    add_token(TOKEN_PROVINCE_ARM, "ARM");
    add_token(TOKEN_PROVINCE_CLY, "CLY");
    add_token(TOKEN_PROVINCE_FIN, "FIN");
    add_token(TOKEN_PROVINCE_GAS, "GAS");
    add_token(TOKEN_PROVINCE_LVN, "LVN");
    add_token(TOKEN_PROVINCE_NAF, "NAF");
    add_token(TOKEN_PROVINCE_PIC, "PIC");
    add_token(TOKEN_PROVINCE_PIE, "PIE");
    add_token(TOKEN_PROVINCE_PRU, "PRU");
    add_token(TOKEN_PROVINCE_SYR, "SYR");
    add_token(TOKEN_PROVINCE_TUS, "TUS");
    add_token(TOKEN_PROVINCE_WAL, "WAL");
    add_token(TOKEN_PROVINCE_YOR, "YOR");
    add_token(TOKEN_PROVINCE_ANK, "ANK");
    add_token(TOKEN_PROVINCE_BEL, "BEL");
    add_token(TOKEN_PROVINCE_BER, "BER");
    add_token(TOKEN_PROVINCE_BRE, "BRE");
    add_token(TOKEN_PROVINCE_CON, "CON");
    add_token(TOKEN_PROVINCE_DEN, "DEN");
    add_token(TOKEN_PROVINCE_EDI, "EDI");
    add_token(TOKEN_PROVINCE_GRE, "GRE");
    add_token(TOKEN_PROVINCE_HOL, "HOL");
    add_token(TOKEN_PROVINCE_KIE, "KIE");
    add_token(TOKEN_PROVINCE_LON, "LON");
    add_token(TOKEN_PROVINCE_LVP, "LVP");
    add_token(TOKEN_PROVINCE_MAR, "MAR");
    add_token(TOKEN_PROVINCE_NAP, "NAP");
    add_token(TOKEN_PROVINCE_NWY, "NWY");
    add_token(TOKEN_PROVINCE_POR, "POR");
    add_token(TOKEN_PROVINCE_ROM, "ROM");
    add_token(TOKEN_PROVINCE_RUM, "RUM");
    add_token(TOKEN_PROVINCE_SEV, "SEV");
    add_token(TOKEN_PROVINCE_SMY, "SMY");
    add_token(TOKEN_PROVINCE_SWE, "SWE");
    add_token(TOKEN_PROVINCE_TRI, "TRI");
    add_token(TOKEN_PROVINCE_TUN, "TUN");
    add_token(TOKEN_PROVINCE_VEN, "VEN");
    add_token(TOKEN_PROVINCE_BUL, "BUL");
    add_token(TOKEN_PROVINCE_SPA, "SPA");
    add_token(TOKEN_PROVINCE_STP, "STP");
    add_token(TOKEN_PARAMETER_UNO, "UNO");
    add_token(TOKEN_COMMAND_MDF, "MDF");
    add_token(TOKEN_COMMAND_MAP, "MAP");
    add_token(TOKEN_COMMAND_OUT, "OUT");
    add_token(TOKEN_PARAMETER_BTL, "BTL");
    add_token(TOKEN_ORDER_NOTE_NSA, "NSA");
    add_token(TOKEN_PRESS_FCT, "FCT");
    add_token(TOKEN_PRESS_WHY, "WHY");
    add_token(TOKEN_PRESS_POB, "POB");
    add_token(TOKEN_PRESS_YDO, "YDO");
    add_token(TOKEN_PRESS_FWD, "FWD");
    add_token(TOKEN_ORDER_NOTE_NVR, "NVR");
    add_token(TOKEN_PARAMETER_DSD, "DSD");
    add_token(TOKEN_COMMAND_ADM, "ADM");
    add_token(TOKEN_COMMAND_SMR, "SMR");
    add_token(TOKEN_PRESS_BCC, "BCC");
    add_token(TOKEN_PRESS_CHO, "CHO");
    add_token(TOKEN_PRESS_UNT, "UNT");
}
