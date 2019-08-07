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
 * Release 8~3
 **/

#ifndef _DAIDE_CLIENT_DAIDE_CLIENT_AI_CLIENT_TYPES_H
#define _DAIDE_CLIENT_DAIDE_CLIENT_AI_CLIENT_TYPES_H

#include <string>

namespace DAIDE {

typedef struct {
    bool ip_specified;              // Whether the IP address was specified
    bool name_specified;            // Whether the server name was specified
    std::string server_name;        // The name of the server machine
    bool port_specified;            // Whether the port number was specified
    uint16_t port_number;           // The port number of the server (in host order)
    bool log_level_specified;       // Whether the log level was specified
    int log_level;                  // The level to log at
    bool reconnection_specified;    // Whether the reconnection parameters have been provided
    std::string reconnect_power;    // Power to reconnect as
    int reconnect_passcode;         // Passcode to reconnect as
} COMMAND_LINE_PARAMETERS;

} // namespace DAIDE

#endif // _DAIDE_CLIENT_DAIDE_CLIENT_AI_CLIENT_TYPES_H
