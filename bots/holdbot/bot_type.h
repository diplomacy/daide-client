/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * Definition of the Bot to use.
 *
 * (C) David Norman 2002 david@ellought.demon.co.uk
 *
 * This software may be reused for non-commercial purposes without charge, and
 * without notifying the author. Use of any part of this software for commercial
 * purposes without permission from the Author is prohibited.
 *
 * Modified by John Newbury
 *
 * Release 8~3
 **/

/*******************************************************************************************
 * Change the #include to specify the header for the main class of your Bot.               *
 * Change the typedef to make BOT_TYPE specify the main class of your Bot.                 *
 * Change the #define BOT_FAMILY to specify the family (generic name) of your bot.         *
 * Change the #define BOT_GENERATION to specify the generation (base version) of your Bot. *
 *******************************************************************************************/

#ifndef _DAIDE_CLIENT_BOTS_HOLDBOT_BOT_TYPE_H
#define _DAIDE_CLIENT_BOTS_HOLDBOT_BOT_TYPE_H

#include "bots/holdbot/holdbot.h"

namespace DAIDE {

using BOT_TYPE = HoldBot;

#define BOT_FAMILY "HoldBot"
#define BOT_GENERATION "8~3"

} // namespace DAIDE

#endif // _DAIDE_CLIENT_BOTS_HOLDBOT_BOT_TYPE_H
