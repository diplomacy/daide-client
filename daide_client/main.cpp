//// AiClient.cpp : Defines the entry point for the application.

//// Copyright (C) 2012, John Newbury. See "Conditions of Use" in johnnewbury.co.cc/diplomacy/conditions-of-use.htm.

//// Release 8~2~b

#include <iostream>
#include <sstream>
#include "ai_client.h"

using DAIDE::BOT_TYPE;
using DAIDE::the_bot;

BOT_TYPE DAIDE::the_bot;

int main(int argc, char *argv[])
{
    std::stringstream sstr;
    for (int i = 0, e = argc; i < e; ++i) {
        sstr << argv[i] << " ";
    }

    if (!the_bot.initialize(sstr.str())) {
        std::cerr << "Couldn't initialize Bot" << std::endl;
        return 1;
    }

    // Main message loop:
    while (the_bot.is_active()) {
        the_bot.m_socket.ReceiveData();

        while (the_bot.OnSocketMessage()) {}
    }

    return 0;
}
