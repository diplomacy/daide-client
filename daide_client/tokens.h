/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * Token Definitions and Class. Token values taken from the Client-Server Protocol Document.
 *
 * (C) David Norman 2002 david@ellought.demon.co.uk
 *
 * This software may be reused for non-commercial purposes without charge, and
 * without notifying the author. Use of any part of this software for commercial
 * purposes without permission from the Author is prohibited.
 *
 * Release 8~3
 **/

#ifndef _DAIDE_CLIENT_DAIDE_CLIENT_TOKENS_H
#define _DAIDE_CLIENT_DAIDE_CLIENT_TOKENS_H

#include "daide_client/types.h"

namespace DAIDE {

using LANGUAGE_TOKEN = uint16_t;
using LANGUAGE_CATEGORY = BYTE;

// Masks
const LANGUAGE_TOKEN NUMBER_MASK = 0xC000;
const LANGUAGE_TOKEN NUMBER_MASK_CHECK = 0;
const LANGUAGE_TOKEN NEGATIVE_MASK = 0x2000;
const LANGUAGE_TOKEN NEGATIVE_MASK_CHECK = 0x2000;
const int MAKE_NEGATIVE_MASK = 0xFFFFE000;

const LANGUAGE_TOKEN PROVINCE_MASK = 0xF800;
const LANGUAGE_TOKEN PROVINCE_MASK_CHECK = 0x5000;

const LANGUAGE_TOKEN ORDER_TURN_MASK = 0xFFF0;
const LANGUAGE_TOKEN ORDER_MOVE_TURN_CHECK = 0x4320;
const LANGUAGE_TOKEN ORDER_RETREAT_TURN_CHECK = 0x4340;
const LANGUAGE_TOKEN ORDER_BUILD_TURN_CHECK = 0x4380;

class TokenMessage;

class Token {
public:
    // The value
    LANGUAGE_TOKEN m_token;

    // Constructor
    Token() : m_token {0} {}
    Token(LANGUAGE_TOKEN token) : m_token {token} {}
    Token(int token) { m_token = token; }
    Token(const Token &token) = default;                                // Copy constructor
    Token(Token &&rhs) = default;                                       // Move constructor
    Token(const LANGUAGE_CATEGORY category, const BYTE sub_category) {
        m_token = (((LANGUAGE_TOKEN) (category)) << 8) | sub_category;
    }
    ~Token() = default;

    // Category and Token
    BYTE get_category() const { return m_token >> 8; }

    BYTE get_subtoken() const { return m_token & 0xFF; }

    // Numbers
    bool is_number() const { return ((m_token & NUMBER_MASK) == NUMBER_MASK_CHECK); }

    int get_number() const {
        return ((m_token & NEGATIVE_MASK) == NEGATIVE_MASK_CHECK) ? m_token | MAKE_NEGATIVE_MASK : m_token;
    }

    void set_number(int number) { m_token = number & (~NUMBER_MASK); }

    // Provinces
    bool is_province() const { return ((m_token & PROVINCE_MASK) == PROVINCE_MASK_CHECK); }

    LANGUAGE_TOKEN get_token() const { return m_token; }

    // Comparison
    bool operator==(const Token &token) const { return (token.m_token == m_token); }

    bool operator!=(const Token &token) const { return (token.m_token != m_token); }

    bool operator<(const Token &token) const { return (m_token < token.m_token); }

    // Assignments
    Token& operator=(const Token &other) = default;                     // Copy Assignment

    Token& operator=(Token &&rhs) = default;                            // Move Assignment

    // Concatenation (code in token_message.cpp)
    TokenMessage operator+(const Token &token) const;

    TokenMessage operator&(const Token &token) const;

    TokenMessage operator+(const TokenMessage &token_message) const;

    TokenMessage operator&(const TokenMessage &token_message) const;
};

// Brackets
const Token TOKEN_OPEN_BRACKET {0x4000};
const Token TOKEN_CLOSE_BRACKET {0x4001};

// Powers
const Token TOKEN_POWER_AUS {0x4100};
const Token TOKEN_POWER_ENG {0x4101};
const Token TOKEN_POWER_FRA {0x4102};
const Token TOKEN_POWER_GER {0x4103};
const Token TOKEN_POWER_ITA {0x4104};
const Token TOKEN_POWER_RUS {0x4105};
const Token TOKEN_POWER_TUR {0x4106};

// Units
const Token TOKEN_UNIT_AMY {0x4200};
const Token TOKEN_UNIT_FLT {0x4201};

// Orders
const Token TOKEN_ORDER_CTO {0x4320};
const Token TOKEN_ORDER_CVY {0x4321};
const Token TOKEN_ORDER_HLD {0x4322};
const Token TOKEN_ORDER_MTO {0x4323};
const Token TOKEN_ORDER_SUP {0x4324};
const Token TOKEN_ORDER_VIA {0x4325};
const Token TOKEN_ORDER_DSB {0x4340};
const Token TOKEN_ORDER_RTO {0x4341};
const Token TOKEN_ORDER_BLD {0x4380};
const Token TOKEN_ORDER_REM {0x4381};
const Token TOKEN_ORDER_WVE {0x4382};

// Order Note
const Token TOKEN_ORDER_NOTE_MBV {0x4400};
const Token TOKEN_ORDER_NOTE_BPR {0x4401};
const Token TOKEN_ORDER_NOTE_CST {0x4402};
const Token TOKEN_ORDER_NOTE_ESC {0x4403};
const Token TOKEN_ORDER_NOTE_FAR {0x4404};
const Token TOKEN_ORDER_NOTE_HSC {0x4405};
const Token TOKEN_ORDER_NOTE_NAS {0x4406};
const Token TOKEN_ORDER_NOTE_NMB {0x4407};
const Token TOKEN_ORDER_NOTE_NMR {0x4408};
const Token TOKEN_ORDER_NOTE_NRN {0x4409};
const Token TOKEN_ORDER_NOTE_NRS {0x440A};
const Token TOKEN_ORDER_NOTE_NSA {0x440B};
const Token TOKEN_ORDER_NOTE_NSC {0x440C};
const Token TOKEN_ORDER_NOTE_NSF {0x440D};
const Token TOKEN_ORDER_NOTE_NSP {0x440E};
const Token TOKEN_ORDER_NOTE_NSU {0x4410};
const Token TOKEN_ORDER_NOTE_NVR {0x4411};
const Token TOKEN_ORDER_NOTE_NYU {0x4412};
const Token TOKEN_ORDER_NOTE_YSC {0x4413};

// Results
const Token TOKEN_RESULT_SUC {0x4500};
const Token TOKEN_RESULT_BNC {0x4501};
const Token TOKEN_RESULT_CUT {0x4502};
const Token TOKEN_RESULT_DSR {0x4503};
const Token TOKEN_RESULT_FLD {0x4504};
const Token TOKEN_RESULT_NSO {0x4505};
const Token TOKEN_RESULT_RET {0x4506};

// Coasts
const Token TOKEN_COAST_NCS {0x4600};
const Token TOKEN_COAST_NEC {0x4602};
const Token TOKEN_COAST_ECS {0x4604};
const Token TOKEN_COAST_SEC {0x4606};
const Token TOKEN_COAST_SCS {0x4608};
const Token TOKEN_COAST_SWC {0x460A};
const Token TOKEN_COAST_WCS {0x460C};
const Token TOKEN_COAST_NWC {0x460E};

// Seasons
const Token TOKEN_SEASON_SPR {0x4700};
const Token TOKEN_SEASON_SUM {0x4701};
const Token TOKEN_SEASON_FAL {0x4702};
const Token TOKEN_SEASON_AUT {0x4703};
const Token TOKEN_SEASON_WIN {0x4704};

// Commands
const Token TOKEN_COMMAND_CCD {0x4800};
const Token TOKEN_COMMAND_DRW {0x4801};
const Token TOKEN_COMMAND_FRM {0x4802};
const Token TOKEN_COMMAND_GOF {0x4803};
const Token TOKEN_COMMAND_HLO {0x4804};
const Token TOKEN_COMMAND_HST {0x4805};
const Token TOKEN_COMMAND_HUH {0x4806};
const Token TOKEN_COMMAND_IAM {0x4807};
const Token TOKEN_COMMAND_LOD {0x4808};
const Token TOKEN_COMMAND_MAP {0x4809};
const Token TOKEN_COMMAND_MDF {0x480A};
const Token TOKEN_COMMAND_MIS {0x480B};
const Token TOKEN_COMMAND_NME {0x480C};
const Token TOKEN_COMMAND_NOT {0x480D};
const Token TOKEN_COMMAND_NOW {0x480E};
const Token TOKEN_COMMAND_OBS {0x480F};
const Token TOKEN_COMMAND_OFF {0x4810};
const Token TOKEN_COMMAND_ORD {0x4811};
const Token TOKEN_COMMAND_OUT {0x4812};
const Token TOKEN_COMMAND_PRN {0x4813};
const Token TOKEN_COMMAND_REJ {0x4814};
const Token TOKEN_COMMAND_SCO {0x4815};
const Token TOKEN_COMMAND_SLO {0x4816};
const Token TOKEN_COMMAND_SND {0x4817};
const Token TOKEN_COMMAND_SUB {0x4818};
const Token TOKEN_COMMAND_SVE {0x4819};
const Token TOKEN_COMMAND_THX {0x481A};
const Token TOKEN_COMMAND_TME {0x481B};
const Token TOKEN_COMMAND_YES {0x481C};
const Token TOKEN_COMMAND_ADM {0x481D};
const Token TOKEN_COMMAND_SMR {0x481E};

// Parameters
const Token TOKEN_PARAMETER_AOA {0x4900};
const Token TOKEN_PARAMETER_BTL {0x4901};
const Token TOKEN_PARAMETER_ERR {0x4902};
const Token TOKEN_PARAMETER_LVL {0x4903};
const Token TOKEN_PARAMETER_MRT {0x4904};
const Token TOKEN_PARAMETER_MTL {0x4905};
const Token TOKEN_PARAMETER_NPB {0x4906};
const Token TOKEN_PARAMETER_NPR {0x4907};
const Token TOKEN_PARAMETER_PDA {0x4908};
const Token TOKEN_PARAMETER_PTL {0x4909};
const Token TOKEN_PARAMETER_RTL {0x490A};
const Token TOKEN_PARAMETER_UNO {0x490B};
const Token TOKEN_PARAMETER_DSD {0x490D};

// Press
const Token TOKEN_PRESS_ALY {0x4A00};
const Token TOKEN_PRESS_AND {0x4A01};
const Token TOKEN_PRESS_BWX {0x4A02};
const Token TOKEN_PRESS_DMZ {0x4A03};
const Token TOKEN_PRESS_ELS {0x4A04};
const Token TOKEN_PRESS_EXP {0x4A05};
const Token TOKEN_PRESS_FCT {0x4A06};
const Token TOKEN_PRESS_FOR {0x4A07};
const Token TOKEN_PRESS_FWD {0x4A08};
const Token TOKEN_PRESS_HOW {0x4A09};
const Token TOKEN_PRESS_IDK {0x4A0A};
const Token TOKEN_PRESS_IFF {0x4A0B};
const Token TOKEN_PRESS_INS {0x4A0C};
const Token TOKEN_PRESS_OCC {0x4A0E};
const Token TOKEN_PRESS_ORR {0x4A0F};
const Token TOKEN_PRESS_PCE {0x4A10};
const Token TOKEN_PRESS_POB {0x4A11};
const Token TOKEN_PRESS_PRP {0x4A13};
const Token TOKEN_PRESS_QRY {0x4A14};
const Token TOKEN_PRESS_SCD {0x4A15};
const Token TOKEN_PRESS_SRY {0x4A16};
const Token TOKEN_PRESS_SUG {0x4A17};
const Token TOKEN_PRESS_THK {0x4A18};
const Token TOKEN_PRESS_THN {0x4A19};
const Token TOKEN_PRESS_TRY {0x4A1A};
const Token TOKEN_PRESS_VSS {0x4A1C};
const Token TOKEN_PRESS_WHT {0x4A1D};
const Token TOKEN_PRESS_WHY {0x4A1E};
const Token TOKEN_PRESS_XDO {0x4A1F};
const Token TOKEN_PRESS_XOY {0x4A20};
const Token TOKEN_PRESS_YDO {0x4A21};
const Token TOKEN_PRESS_CHO {0x4A22};
const Token TOKEN_PRESS_BCC {0x4A23};
const Token TOKEN_PRESS_UNT {0x4A24};

// Provinces
const Token TOKEN_PROVINCE_BOH {0x5000};
const Token TOKEN_PROVINCE_BUR {0x5001};
const Token TOKEN_PROVINCE_GAL {0x5002};
const Token TOKEN_PROVINCE_RUH {0x5003};
const Token TOKEN_PROVINCE_SIL {0x5004};
const Token TOKEN_PROVINCE_TYR {0x5005};
const Token TOKEN_PROVINCE_UKR {0x5006};
const Token TOKEN_PROVINCE_BUD {0x5107};
const Token TOKEN_PROVINCE_MOS {0x5108};
const Token TOKEN_PROVINCE_MUN {0x5109};
const Token TOKEN_PROVINCE_PAR {0x510A};
const Token TOKEN_PROVINCE_SER {0x510B};
const Token TOKEN_PROVINCE_VIE {0x510C};
const Token TOKEN_PROVINCE_WAR {0x510D};
const Token TOKEN_PROVINCE_ADR {0x520E};
const Token TOKEN_PROVINCE_AEG {0x520F};
const Token TOKEN_PROVINCE_BAL {0x5210};
const Token TOKEN_PROVINCE_BAR {0x5211};
const Token TOKEN_PROVINCE_BLA {0x5212};
const Token TOKEN_PROVINCE_EAS {0x5213};
const Token TOKEN_PROVINCE_ECH {0x5214};
const Token TOKEN_PROVINCE_GOB {0x5215};
const Token TOKEN_PROVINCE_GOL {0x5216};
const Token TOKEN_PROVINCE_HEL {0x5217};
const Token TOKEN_PROVINCE_ION {0x5218};
const Token TOKEN_PROVINCE_IRI {0x5219};
const Token TOKEN_PROVINCE_MAO {0x521A};
const Token TOKEN_PROVINCE_NAO {0x521B};
const Token TOKEN_PROVINCE_NTH {0x521C};
const Token TOKEN_PROVINCE_NWG {0x521D};
const Token TOKEN_PROVINCE_SKA {0x521E};
const Token TOKEN_PROVINCE_TYS {0x521F};
const Token TOKEN_PROVINCE_WES {0x5220};
const Token TOKEN_PROVINCE_ALB {0x5421};
const Token TOKEN_PROVINCE_APU {0x5422};
const Token TOKEN_PROVINCE_ARM {0x5423};
const Token TOKEN_PROVINCE_CLY {0x5424};
const Token TOKEN_PROVINCE_FIN {0x5425};
const Token TOKEN_PROVINCE_GAS {0x5426};
const Token TOKEN_PROVINCE_LVN {0x5427};
const Token TOKEN_PROVINCE_NAF {0x5428};
const Token TOKEN_PROVINCE_PIC {0x5429};
const Token TOKEN_PROVINCE_PIE {0x542A};
const Token TOKEN_PROVINCE_PRU {0x542B};
const Token TOKEN_PROVINCE_SYR {0x542C};
const Token TOKEN_PROVINCE_TUS {0x542D};
const Token TOKEN_PROVINCE_WAL {0x542E};
const Token TOKEN_PROVINCE_YOR {0x542F};
const Token TOKEN_PROVINCE_ANK {0x5530};
const Token TOKEN_PROVINCE_BEL {0x5531};
const Token TOKEN_PROVINCE_BER {0x5532};
const Token TOKEN_PROVINCE_BRE {0x5533};
const Token TOKEN_PROVINCE_CON {0x5534};
const Token TOKEN_PROVINCE_DEN {0x5535};
const Token TOKEN_PROVINCE_EDI {0x5536};
const Token TOKEN_PROVINCE_GRE {0x5537};
const Token TOKEN_PROVINCE_HOL {0x5538};
const Token TOKEN_PROVINCE_KIE {0x5539};
const Token TOKEN_PROVINCE_LON {0x553A};
const Token TOKEN_PROVINCE_LVP {0x553B};
const Token TOKEN_PROVINCE_MAR {0x553C};
const Token TOKEN_PROVINCE_NAP {0x553D};
const Token TOKEN_PROVINCE_NWY {0x553E};
const Token TOKEN_PROVINCE_POR {0x553F};
const Token TOKEN_PROVINCE_ROM {0x5540};
const Token TOKEN_PROVINCE_RUM {0x5541};
const Token TOKEN_PROVINCE_SEV {0x5542};
const Token TOKEN_PROVINCE_SMY {0x5543};
const Token TOKEN_PROVINCE_SWE {0x5544};
const Token TOKEN_PROVINCE_TRI {0x5545};
const Token TOKEN_PROVINCE_TUN {0x5546};
const Token TOKEN_PROVINCE_VEN {0x5547};
const Token TOKEN_PROVINCE_BUL {0x5748};
const Token TOKEN_PROVINCE_SPA {0x5749};
const Token TOKEN_PROVINCE_STP {0x574A};

// Categories
const LANGUAGE_CATEGORY CATEGORY_BRACKET = 0x40;
const LANGUAGE_CATEGORY CATEGORY_POWER = 0x41;
const LANGUAGE_CATEGORY CATEGORY_UNIT = 0x42;
const LANGUAGE_CATEGORY CATEGORY_ORDER = 0x43;
const LANGUAGE_CATEGORY CATEGORY_ORDER_NOTE = 0x44;
const LANGUAGE_CATEGORY CATEGORY_RESULT = 0x45;
const LANGUAGE_CATEGORY CATEGORY_COAST = 0x46;
const LANGUAGE_CATEGORY CATEGORY_SEASON = 0x47;
const LANGUAGE_CATEGORY CATEGORY_COMMAND = 0x48;
const LANGUAGE_CATEGORY CATEGORY_PARAMETER = 0x49;
const LANGUAGE_CATEGORY CATEGORY_PRESS = 0x4A;
const LANGUAGE_CATEGORY CATEGORY_ASCII = 0x4B;

const LANGUAGE_CATEGORY CATEGORY_NUMBER_MIN = 0x00;
const LANGUAGE_CATEGORY CATEGORY_NUMBER_MAX = 0x3F;

const LANGUAGE_CATEGORY CATEGORY_PROVINCE_MIN = 0x50;
const LANGUAGE_CATEGORY CATEGORY_PROVINCE_MAX = 0x57;

// Number range
const int MAX_NEGATIVE_NUMBER = -8192;
const int MAX_POSITIVE_NUMBER = 8191;

// Tokens local to a machine (must be 0x5800 to 0x5FFF)
const Token TOKEN_END_OF_MESSAGE {0x5FFF};

} // namespace DAIDE

#endif // _DAIDE_CLIENT_DAIDE_CLIENT_TOKENS_H
