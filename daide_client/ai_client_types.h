/**
 * Diplomacy AI Client - Part of the DAIDE project.
 *
 * Types used across different files.
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

#ifndef _DAN_TYPES_H
#define _DAN_TYPES_H

#include "string.h"

typedef struct {
    bool ip_specified;            // Whether the IP address was specified
    bool name_specified;        // Whether the server name was specified
    String server_name;            // The name of the server machine
    bool port_specified;        // Whether the port number was specified
    unsigned short port_number;    // The port number of the server (in host order)
    bool log_level_specified;    // Whether the log level was specified
    int log_level;                // The level to log at
    bool reconnection_specified;// Whether the reconnection parameters have been provided
    String reconnect_power;        // Power to reconnect as
    int reconnect_passcode;        // Passcode to reconnect as
} COMMAND_LINE_PARAMETERS;

#endif // _DAN_TYPES_H
