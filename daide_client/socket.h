// JPN_Socket.h: interface for the JPN::Socket class and related material.

// Copyright (C) 2012, John Newbury. See "Conditions of Use" in johnnewbury.co.cc/diplomacy/conditions-of-use.htm.

// Release 8~2~b

/////////////////////////////////////////////////////////////////////////////

#ifndef _DAIDE_CLIENT_DAIDE_CLIENT_SOCKET_H
#define _DAIDE_CLIENT_DAIDE_CLIENT_SOCKET_H

#include <queue>

#include <sys/types.h>
#include <sys/socket.h>

#include "windaide_symboles.h"

namespace DAIDE {

class MessageHeader {           // Message header, common to all types.
public:
    char type;                  // type of message
    char pad;                   // unused; maintains 16-bit alignment
    int16_t length;             // length of body in bytes, which follows the header
};

class Socket {
    // Message-oriented socket.

    using MessageQueue = std::queue<char *>;

    SOCKET MySocket;

    static Socket *SocketTab[FD_SETSIZE];               // table of active Socket*
    static int SocketCnt;                               // # active Socket

    MessageQueue IncomingMessageQueue;                  // queue of incoming messages
    MessageQueue OutgoingMessageQueue;                  // queue of outgoing messages
    char *IncomingMessage {nullptr};                    // current incoming message; points to Header until length read;
                                                        // else a full, but incomplete, message
    char *OutgoingMessage {nullptr};                    // current outgoing message, when partially sent; else 0
    int IncomingNext;                                   // index of start of next incoming message in buffer
    int OutgoingNext;                                   // index of start of next outgoingmessage in buffer
    int IncomingLength;                                 // whole length of current incoming message, including header
    int OutgoingLength;                                 // whole length of current outgoing message, including header
    MessageHeader Header;                               // buffer for header of current incoming message
                                                        // pending new IncomingMessage, when length is known
    bool Connected {false};                             // true iff connected

    void InsertSocket();

    void RemoveSocket();

    void SendData();

    void ReceiveData();

    void Start();

    void PushIncomingMessage(char *message);

    virtual void OnConnect(int error);

    virtual void OnClose(int error);

    virtual void OnReceive(int error);

    virtual void OnSend(int error);

public:
    Socket() : IncomingMessage(nullptr), OutgoingMessage(nullptr) {}

    virtual ~Socket();

    virtual bool Connect(const char *address, int port);

    char *PullIncomingMessage();

    void PushOutgoingMessage(char *message);

    static Socket **FindSocket(int socket);

    static void AdjustOrdering(int16_t &x);

    static void AdjustOrdering(char *message, int16_t length);
};

/////////////////////////////////////////////////////////////////////////////

} // namespace DAIDE

#endif // _DAIDE_CLIENT_DAIDE_CLIENT_SOCKET_H
