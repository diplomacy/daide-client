cmake_minimum_required(VERSION 3.14)
project(daide_client)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(daide_client
        bots/basebot/bot_type.h
        bots/dumbbot/bot_type.h
        bots/dumbbot/dumbbot.cpp
        bots/dumbbot/dumbbot.h
        bots/holdbot/bot_type.h
        bots/holdbot/holdbot.cpp
        bots/holdbot/holdbot.h
        bots/randbot/bot_type.h
        bots/randbot/randbot.cpp
        bots/randbot/randbot.h
        daide_client/adjudicator.cpp
        daide_client/ai_client.cpp
        daide_client/ai_client.h
        daide_client/ai_client_types.h
        daide_client/base_bot.cpp
        daide_client/base_bot.h
        daide_client/bot_type.h
        daide_client/error_log.cpp
        daide_client/error_log.h
        daide_client/map_and_units.cpp
        daide_client/map_and_units.h
        daide_client/socket.cpp
        daide_client/socket.h
        daide_client/token_message.cpp
        daide_client/token_message.h
        daide_client/token_text_map.cpp
        daide_client/token_text_map.h
        daide_client/tokens.h
        daide_client/types.h)
