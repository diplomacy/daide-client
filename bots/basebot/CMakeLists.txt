cmake_minimum_required(VERSION 3.14)
project(base_bot)

set(CMAKE_CXX_STANDARD 14)
set(SRC_DIR ../..)

include_directories(.)
include_directories(${SRC_DIR})

add_executable(base_bot

    ${SRC_DIR}/daide_client/main.cpp
    ${SRC_DIR}/daide_client/adjudicator.cpp
    ${SRC_DIR}/daide_client/base_bot.cpp
    ${SRC_DIR}/daide_client/error_log.cpp
    ${SRC_DIR}/daide_client/map_and_units.cpp
    ${SRC_DIR}/daide_client/socket.cpp
    ${SRC_DIR}/daide_client/token_message.cpp
    ${SRC_DIR}/daide_client/token_text_map.cpp
    ${SRC_DIR}/daide_client/windaide_symboles.cpp)
