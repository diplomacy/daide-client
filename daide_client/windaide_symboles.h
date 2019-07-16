#ifndef BRIDGE_H
#define BRIDGE_H

#include <cassert>
#include <cerrno>

#define ASSERT assert
#define BOOL bool

#define SOCKET int
#define SOCKET_ERROR ssize_t(-1)

#define WSAEWOULDBLOCK EWOULDBLOCK

#define ioctlsocket ioctl

int WSAGetLastError() { return errno; }

#endif // BRIDGE_H
