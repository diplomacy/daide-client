/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * Error Logging Functions.
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

#ifndef _DAIDE_CLIENT_DAIDE_CLIENT_ERROR_LOG_H
#define _DAIDE_CLIENT_DAIDE_CLIENT_ERROR_LOG_H

#include "daide_client/token_message.h"

namespace DAIDE {

void enable_logging(bool enable);

FILE *open(const char *filename, const char *mode);

void log(const char *format, ...);

void log_error(const char *format, ...);

void log_daide_message(bool is_incoming, const TokenMessage &message);

void close_logs();

} // namespace DAIDE

#endif // _DAIDE_CLIENT_DAIDE_CLIENT_ERROR_LOG_H
