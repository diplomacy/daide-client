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
 * Release 8~2
 **/

#ifndef _DIPAI_ERROR_LOG_H
#define _DIPAI_ERROR_LOG_H

#include "token_message.h"

void enable_logging(bool enable);

void log(char *format, ...);

void log_error(char *error, ...);

void log_daide_message(bool is_incoming, TokenMessage &message);

void close_logs();

int display(const char *message, const char *caption = nullptr, int type = MB_OK);

#endif

