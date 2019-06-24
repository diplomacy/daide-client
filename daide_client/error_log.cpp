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
 * Release 8~2
 **/

#include <cstdarg>
#include "error_log.h"

// TODO - REWRITE completely
// Avoid using variadic functions
// Also log to file

FILE *bad_log = nullptr;
FILE *big_log = nullptr;
bool logging_enabled = true;

const char BAD_LOG_FILENAME[] = "badlog.txt";
const char BIG_LOG_FILENAME[] = "biglog.txt";

void enable_logging(bool enable) { logging_enabled = enable; }

FILE *open(const char *filename, const char *mode) { return fopen(filename, mode); }

void log(const std::string &format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    const int size = 10000;
    char buffer[size];

#if    WINVER >= 0x0600
    if (vsprintf_s( buffer, size, format, arg_list ) < 0) throw "Buffer ovrflow in `log`";
#else
    vsprintf(buffer, format, arg_list); // hope `buffer` is big enough!
#endif

    if (logging_enabled) {
        if (big_log == nullptr) {
            big_log = open(BIG_LOG_FILENAME, "w");
        }

        if (big_log != nullptr) {
            fprintf(big_log, "== %s\n", buffer);

            fflush(big_log);
        }
    }

    va_end(arg_list);
}

void log_error(const std::string &message) {
    // Fix me
}

void log_error(char *format, ...) {
    /** va_list arg_list;

    va_start(arg_list, format);

    if (bad_log == nullptr) {
        bad_log = open(BAD_LOG_FILENAME, "w");
    }

    if (bad_log != nullptr) {
        vfprintf(bad_log, format, arg_list);
        fprintf(bad_log, "\n");

        fflush(bad_log);
    }

    // Send message to normal log too, so that it appears in context.
    const int size = 10000;
    char buffer[size];

#if    WINVER >= 0x0600
    if ( vsprintf_s( buffer, size, format, arg_list ) < 0) throw "Buffer overflow in `log_error`";
#else
    vsprintf(buffer, format, arg_list); // hope `buffer` is big enough!
#endif

    log(buffer);

    va_end(arg_list);
    */
}

void log_daide_message(bool is_incoming, const DAIDE::TokenMessage &message) {
    if (logging_enabled) {
        if (big_log == nullptr) {
            big_log = open(BIG_LOG_FILENAME, "w");
        }

        if (big_log != nullptr) {
            if (is_incoming) {
                fprintf(big_log, ">> ");
            } else {
                fprintf(big_log, "<< ");
            }

            fprintf(big_log, "%s\n", message.get_message_as_text());
            fflush(big_log);
        }
    }
}

void close_logs() {
    if (bad_log != nullptr) {
        fclose(bad_log);
    }

    if (big_log != nullptr) {
        fclose(big_log);
    }
}
