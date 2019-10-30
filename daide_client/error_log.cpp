/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * Error Logging
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

#include <cstdarg>
#include <cstdio>
#include <vector>
#include "error_log.h"

// TODO - REWRITE completely
// Avoid using variadic functions
// Also log to file

using namespace DAIDE;

static FILE *bad_log = nullptr;
static FILE *big_log = nullptr;
static bool logging_enabled = true;

static const char *BAD_LOG_FILENAME = "badlog.txt";
static const char *BIG_LOG_FILENAME = "biglog.txt";

void DAIDE::enable_logging(bool enable) { logging_enabled = enable; }

FILE *DAIDE::open(const char *filename, const char *mode) { return fopen(filename, mode); }

FILE *_get_bad_log() {
    if (bad_log == nullptr) {
        bad_log = open(BAD_LOG_FILENAME, "w");
    }
    return bad_log;
}

FILE *_get_big_log() {
    if (logging_enabled && big_log == nullptr) {
        big_log = open(BIG_LOG_FILENAME, "w");
    }
    return big_log;
}

void _log(FILE *file, const char *log, const std::string &prefix = "== ")
{
    if (file != nullptr) {
        fprintf(file, (prefix + "%s\n").c_str(), log);
        fflush(file);
    }
}

void DAIDE::log(const char *format, ...) {
    // initialize use of the variable argument array
    va_list arg_list;
    va_start(arg_list, format);

    // reliably acquire the size from a copy of
    // the variable argument array
    // and a functionally reliable call
    // to mock the formatting
    va_list vaCopy;
    va_copy(vaCopy, arg_list);
    const size_t length = static_cast<size_t>(std::vsnprintf(nullptr, 0, format, vaCopy));
    va_end(vaCopy);

    // return a formatted string without
    // risking memory mismanagement
    // and without assuming any compiler
    // or platform specific behavior
    std::vector<char> buffer(length + 1);
    std::vsnprintf(buffer.data(), buffer.size(), format, arg_list);
    va_end(arg_list);

    _log(_get_big_log(), buffer.data(), "== ");
}

void DAIDE::log_error(const char* format, ...) {
    // initialize use of the variable argument array
    va_list arg_list;
    va_start(arg_list, format);

    // reliably acquire the size from a copy of
    // the variable argument array
    // and a functionally reliable call
    // to mock the formatting
    va_list vaCopy;
    va_copy(vaCopy, arg_list);
    const size_t length = static_cast<size_t>(std::vsnprintf(nullptr, 0, format, vaCopy));
    va_end(vaCopy);

    // return a formatted string without
    // risking memory mismanagement
    // and without assuming any compiler
    // or platform specific behavior
    std::vector<char> buffer(length + 1);
    std::vsnprintf(buffer.data(), buffer.size(), format, arg_list);
    va_end(arg_list);

    _log(_get_bad_log(), buffer.data(), "");
    _log(_get_big_log(), buffer.data(), "== ");
}

void DAIDE::log_daide_message(bool is_incoming, const DAIDE::TokenMessage &message) {
    _log(_get_big_log(), message.get_message_as_text().c_str(), is_incoming ? ">> " : "<< ");
}

void DAIDE::close_logs() {
    if (bad_log != nullptr) {
        fclose(bad_log);
        bad_log = nullptr;
    }

    if (big_log != nullptr) {
        fclose(big_log);
        bad_log = nullptr;
    }
}
