#ifndef WINDAIDE_SYMBOLES_H
#define WINDAIDE_SYMBOLES_H

#include <cassert>
#include <cerrno>

#define ASSERT assert
#define BOOL bool

#define SOCKET int
#define SOCKET_ERROR ssize_t(-1)

#define WSAEWOULDBLOCK EWOULDBLOCK

#define ioctlsocket ioctl

int WSAGetLastError();

#endif // WINDAIDE_SYMBOLES_H
