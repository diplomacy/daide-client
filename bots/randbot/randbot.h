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

#ifndef DAIDE_CLIENT_BOTS_RANDBOT_RANDBOT_H
#define DAIDE_CLIENT_BOTS_RANDBOT_RANDBOT_H

#include "daide_client/base_bot.h"

class RandBot : public DAIDE::BaseBot {
    RandBot(const RandBot &other) = delete;                 // Copy constructor
    RandBot(RandBot &&rhs) = delete;                        // Move constructor

    RandBot& operator=(const RandBot &other) = delete;      // Copy Assignment
    RandBot& operator=(RandBot &&rhs) = delete;             // Move Assignment

public:
    RandBot() = default;
    ~RandBot() override = default;

    void send_nme_or_obs() override;

    void process_now_message(const DAIDE::TokenMessage &incoming_msg) override;
};

#endif // DAIDE_CLIENT_BOTS_RANDBOT_RANDBOT_H
