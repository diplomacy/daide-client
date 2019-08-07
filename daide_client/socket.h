// JPN_Socket.h: interface for the JPN::Socket class and related material.

// Copyright (C) 2012, John Newbury. See "Conditions of Use" in johnnewbury.co.cc/diplomacy/conditions-of-use.htm.

// Release 8~3

/////////////////////////////////////////////////////////////////////////////

#ifndef _DAIDE_CLIENT_DAIDE_CLIENT_SOCKET_H
#define _DAIDE_CLIENT_DAIDE_CLIENT_SOCKET_H

#include <memory>
#include <queue>

#include <sys/types.h>
#include <sys/socket.h>

#include "daide_client/windaide_symbols.h"

namespace DAIDE {

class MessageHeader {           // Message header, common to all types.
public:
    char type;                  // type of message
    char pad;                   // unused; maintains 16-bit alignment
    int16_t length;             // length of body in bytes, which follows the header
};

class Socket {
    // Message-oriented socket.
public:
    using MessagePtr = std::shared_ptr<char>;

private:
    using MessageQueue = std::queue<MessagePtr>;

    SOCKET MySocket;

    static Socket* SocketTab[FD_SETSIZE];               // table of active Socket*
    static int SocketCnt;                               // # active Socket

    MessageQueue IncomingMessageQueue;                  // queue of incoming messages
    MessageQueue OutgoingMessageQueue;                  // queue of outgoing messages
    MessagePtr IncomingMessage {nullptr};               // current incoming message; points to Header until length read;
                                                        // else a full, but incomplete, message
    MessagePtr OutgoingMessage {nullptr};               // current outgoing message, when partially sent; else 0
    size_t IncomingNext;                                // index of start of next incoming message in buffer
    size_t OutgoingNext;                                // index of start of next outgoingmessage in buffer
    size_t IncomingLength;                              // whole length of current incoming message, including header
    size_t OutgoingLength;                              // whole length of current outgoing message, including header
    const MessagePtr HeaderDataPtr;
    MessageHeader* const Header;                        // buffer for header of current incoming message
                                                        // pending new IncomingMessage, when length is known
    bool Connected {false};                             // true iff connected

    void InsertSocket();

    void RemoveSocket();

    void PushIncomingMessage(const MessagePtr &message);

public:
    Socket() :
        IncomingMessage(nullptr),
        OutgoingMessage(nullptr),
        HeaderDataPtr(new char[sizeof(MessageHeader)]),
        Header(reinterpret_cast<MessageHeader*>(HeaderDataPtr.get())) {}
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    virtual ~Socket();

    virtual bool Connect(const std::string& address, int port);

    void Start();

    void Close();

    void SendData();

    void ReceiveData();

    MessagePtr PullIncomingMessage();

    void PushOutgoingMessage(const MessagePtr &message);

    static Socket** FindSocket(SOCKET socket);

    static void AdjustOrdering(int16_t &x);

    static void AdjustOrdering(const MessagePtr &message, int16_t length);
};

Socket::MessagePtr make_message(int length);

MessageHeader* get_message_header(const Socket::MessagePtr &message);

template <typename T>
T* get_message_content(const Socket::MessagePtr &message) {
    return reinterpret_cast<T*>(message.get() + sizeof(MessageHeader));
}

/////////////////////////////////////////////////////////////////////////////

} // namespace DAIDE

#endif // _DAIDE_CLIENT_DAIDE_CLIENT_SOCKET_H
