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
 * Release 8~2~b
 **/

#ifndef DAIDE_CLIENT_BOTS_HOLDBOT_HOLDBOT_H
#define DAIDE_CLIENT_BOTS_HOLDBOT_HOLDBOT_H

#include "daide_client/base_bot.h"

class HoldBot : public DAIDE::BaseBot {
    HoldBot(const HoldBot &other) = delete;                 // Copy constructor
    HoldBot(HoldBot &&rhs) = delete;                        // Move constructor

    HoldBot& operator=(const HoldBot &other) = delete;      // Copy Assignment
    HoldBot& operator=(HoldBot &&rhs) = delete;             // Move Assignment

public:
    HoldBot() = default;
    ~HoldBot() override = default;

    void send_nme_or_obs() override;

    void process_now_message(const DAIDE::TokenMessage &incoming_msg) override;
};

#endif // DAIDE_CLIENT_BOTS_HOLDBOT_HOLDBOT_H
