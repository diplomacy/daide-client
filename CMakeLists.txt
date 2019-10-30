cmake_minimum_required(VERSION 3.14)
project(daide_client)

set(CMAKE_CXX_STANDARD 14)
set(SRC_DIR .)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# -----------------------
# Includes
# -----------------------
set(COMMON_DAIDE_CLIENT
        ${SRC_DIR}/bots/basebot/basebot.cpp
        ${SRC_DIR}/daide_client/main.cpp
        ${SRC_DIR}/daide_client/adjudicator.cpp
        ${SRC_DIR}/daide_client/error_log.cpp
        ${SRC_DIR}/daide_client/map_and_units.cpp
        ${SRC_DIR}/daide_client/socket.cpp
        ${SRC_DIR}/daide_client/token_message.cpp
        ${SRC_DIR}/daide_client/token_text_map.cpp
        ${SRC_DIR}/daide_client/windaide_symbols.cpp)

# -----------------------
# Bots
# -----------------------
add_executable(dumbbot
        ${SRC_DIR}/bots/dumbbot/dumbbot.cpp
        ${COMMON_DAIDE_CLIENT})
target_include_directories(dumbbot PUBLIC ${SRC_DIR}/bots/dumbbot ${SRC_DIR}/bots/basebot ${SRC_DIR})

add_executable(holdbot
        ${SRC_DIR}/bots/holdbot/holdbot.cpp
        ${COMMON_DAIDE_CLIENT})
target_include_directories(holdbot PUBLIC ${SRC_DIR}/bots/holdbot ${SRC_DIR}/bots/basebot ${SRC_DIR})

add_executable(randbot
        ${SRC_DIR}/bots/randbot/randbot.cpp
        ${COMMON_DAIDE_CLIENT})
target_include_directories(randbot PUBLIC ${SRC_DIR}/bots/randbot ${SRC_DIR}/bots/basebot ${SRC_DIR})
