// JPN_Socket.cpp: implementation of the JPN::Socket class and related material.

// Copyright (C) 2012, John Newbury. See "Conditions of Use" in johnnewbury.co.cc/diplomacy/conditions-of-use.htm.

// Release 8~3

/////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "daide_client/ai_client.h"
#include "daide_client/error_log.h"
#include "daide_client/socket.h"

/////////////////////////////////////////////////////////////////////////////

using DAIDE::Socket;
using DAIDE::MessageHeader;

using MessagePtr = Socket::MessagePtr;

Socket* Socket::SocketTab[FD_SETSIZE];

int Socket::SocketCnt;

Socket::~Socket() {
    // Destructor.
    // FIXME - Avoid using C-casts and delete
    RemoveSocket();
    OutgoingMessage.reset();
    IncomingMessage.reset();
    if (!OutgoingMessageQueue.empty()) {
        MessageQueue empty;
        std::swap(OutgoingMessageQueue, empty);
    }
    if (!IncomingMessageQueue.empty()) {
        MessageQueue empty;
        std::swap(IncomingMessageQueue, empty);
    }
}

void Socket::InsertSocket() {
    // Insert `this` into SocketTab. Must not already be present; should be full, else system socket table would already be full.
    ASSERT(!FindSocket(MySocket) && SocketCnt < FD_SETSIZE);
    SocketTab[SocketCnt++] = this;
}

void Socket::RemoveSocket() {
    // Remove `this` from SocketTab iff present.
    Socket** s = FindSocket(MySocket);
    if (s) *s = SocketTab[--SocketCnt]; // found; replace by last elem
}

void Socket::SendData() {
    // Send all available data to socket, while space is available.
    ASSERT(Connected);
    for (;;) { // while data available to send and space avalable
        if (!OutgoingMessage) { // no outgoing message in progress
            if (OutgoingMessageQueue.empty()) return; // nothing more to send
            OutgoingMessage = OutgoingMessageQueue.front(); // next message to send
            OutgoingMessageQueue.pop();
            OutgoingNext = 0;
            short length = get_message_header(OutgoingMessage)->length;
            AdjustOrdering(OutgoingMessage, length);
            OutgoingLength = sizeof(MessageHeader) + static_cast<size_t>(length);
        }

        // # bytes sent, or SOCKET_ERROR
        ssize_t sent = send(MySocket, OutgoingMessage.get() + OutgoingNext, OutgoingLength - OutgoingNext, 0);

        if (sent == SOCKET_ERROR) {
            int error = WSAGetLastError();
            log_error("Failure %d during SendDatar", error);
            DAIDE::the_bot.stop();
            return;
        }
        OutgoingNext += static_cast<size_t>(sent);
        if (OutgoingNext < OutgoingLength) return; // current message not fully sent
        OutgoingMessage.reset();
    }
}

void Socket::ReceiveData() {
    // Receive all data available from socket.
    ASSERT(Connected);

    const size_t bufferLength = 1024; // arbitrary
    char buffer[bufferLength];
    size_t bufferNext = 0; // index of next byte in `buffer`
    ssize_t received = recv(MySocket, buffer, bufferLength, 0);

    if (!received) {
        log_error("Failure: closed socket during read from Server");
        the_bot.stop();
        return;
    }
    if (received == SOCKET_ERROR) {
        int error = WSAGetLastError();
        log_error("Failure %d during ReceiveData", error);
        the_bot.stop();
        return;
    }
    for (;;) { // while incoming data available
        // max # bytes that may be read into current message
        size_t count = std::min(IncomingLength - IncomingNext, static_cast<size_t>(received) - bufferNext);
        if (!count) return; // no more data available

        // copy `count` bytes from `buffer` to IncomingMessage
        ASSERT(count > 0);
        memcpy(IncomingMessage.get() + IncomingNext, buffer + bufferNext, count);

        IncomingNext += count;
        bufferNext += count;

        if (IncomingNext == sizeof(MessageHeader)) { // just completed reading Header; length now known
            MessageHeader* message_header = get_message_header(IncomingMessage);
            short length = message_header->length; // length of body
            AdjustOrdering(length);

            IncomingMessage = make_message(length);
            IncomingLength = sizeof(MessageHeader) + static_cast<size_t>(length); // full message length

            // Copy Header to full IncomingMessage
            message_header = get_message_header(IncomingMessage);
            message_header->type = Header->type;
            message_header->pad = Header->pad;
            message_header->length = Header->length;

            IncomingNext = sizeof(MessageHeader);
        }

        if (IncomingNext >= IncomingLength) { // current incoming message is complete
            AdjustOrdering(IncomingMessage, static_cast<short>(IncomingLength - sizeof(MessageHeader)));
            PushIncomingMessage(IncomingMessage);

            // Make IncomingMessage use Header, pending known length of next full message.
            IncomingMessage = HeaderDataPtr;
            IncomingNext = 0;
            IncomingLength = sizeof(MessageHeader);
        }
    }
}

void Socket::Start() {
    // Start using the socket.
    Connected = true;
    log("connected");
    SendData();
}

void Socket::Close()
{
    Connected = false;
    if (MySocket) {
        log("disconnected");
        close(MySocket);
        MySocket = 0;
    }
}

bool Socket::Connect(const std::string& address, int port) {
    // Initiate asynchronous connection of socket to `address` and `port`; return true iff OK.
    MySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (MySocket < 0) {
        int error = WSAGetLastError();
        log_error("Failure %d during socket: %s", error, strerror(error));
        return false;
    }

//    unsigned long mode = 1; // non-blocking
//    if (ioctlsocket(MySocket, FIONBIO, &mode)) {
//        int error = WSAGetLastError();
//        log_error("Failure %d during ioctlsocket: %s", error, strerror(error));
//        return false;
//    }

    int val = true;
    if (setsockopt(MySocket, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val))) {
        int error = WSAGetLastError();
        log_error("Failure %d during setsockopt: %s", error, strerror(error));
        return false;
    }

    // Prepare `saPtr` and `saLen` args for connect, from `server` and `port`. Requires mystical incantations!
    // Assumed fast enough to do synchronously, albeit may use external name server;
    // else needs separate thread!
    sockaddr* saPtr;
    struct sockaddr_in sa {};
    socklen_t saLen;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(address.c_str());

    if (sa.sin_addr.s_addr == INADDR_NONE) { // not valid IP number
        log_error("Invalid IP address %s", address.c_str());
        return false;
    }

    saPtr = reinterpret_cast<sockaddr*>(&sa);
    saLen = sizeof(sa);

    // Connect to server.
    if (connect(MySocket, saPtr, saLen)) {
        int error = WSAGetLastError();
        log_error("Failure %d during Connect: %s", error, strerror(error));
        Close();
        return false;
    }

    // Make IncomingMessage use Header, pending known length of full message.
    IncomingMessage = HeaderDataPtr;
    IncomingNext = 0;
    IncomingLength = sizeof(MessageHeader);
    OutgoingMessage.reset();

    InsertSocket();

    return true;
}

void Socket::PushIncomingMessage(const MessagePtr &message) {
    // Push incoming `message` on end of IncomingMessageQueue.
    IncomingMessageQueue.push(message); // safe to follow the post as we are in the Main thread
}

void Socket::PushOutgoingMessage(const MessagePtr &message) {
    // Push outgoing `message` on end of OutgoingMessageQueue.
    OutgoingMessageQueue.push(message);
    if (!OutgoingMessage && Connected) SendData();
}

Socket::MessagePtr Socket::PullIncomingMessage() {
    // Pull next message from front of IncomingMessageQueue. Post a new message event if more messages remain on queue.

    // ASSERT(!IncomingMessageQueue.empty());
    MessagePtr incomingMessage = nullptr;

    if (!IncomingMessageQueue.empty()) {
        incomingMessage = IncomingMessageQueue.front(); // message to be processed
        IncomingMessageQueue.pop(); // remove it
        // trigger recall if more messages remain
    }

    return incomingMessage;
}

Socket** Socket::FindSocket(int socket) {
    // Return the Socket** in SocketTab that owns `socket`; 0 iff not found.
    Socket** s;
    for (int i = 0; i < SocketCnt; ++i) {
        s = &SocketTab[i];
        if ((*s)->MySocket == socket) return s;
    }
    return nullptr;
}

void Socket::AdjustOrdering(short &x) {
    // Adjust the byte ordering of `x`, to or from network ordering of a `short`.

#if LITTLE_ENDIAN
    ASSERT(htons(1) != 1);

    x = static_cast<short>((x << 8) | (static_cast<uint16_t>(x) >> 8));
#else
    ASSERT(htons(1) == 1);
#endif
}

void Socket::AdjustOrdering(const MessagePtr &message, short length) {
    // Adjust 16-bit aligned `message`, having body `length`, to or from network ordering of its components.
    // 'length` must be specified, as that in the header may or may not be in internal order.
    // Header `type` and `pad` are single byte, so need no reordering; the rest are byte-pair `short`, so may need reordering.

#if LITTLE_ENDIAN
    MessageHeader* message_header = get_message_header(message);
    auto* message_content = get_message_content<short>(message);
    AdjustOrdering(message_header->length);
    for (int i = length / static_cast<short>(sizeof(short)) - 1; i >= 0; i--) {
        AdjustOrdering(message_content[i]);
    }
#endif
}

MessagePtr DAIDE::make_message(int length) {
    MessagePtr message = MessagePtr(new char[sizeof(MessageHeader) + static_cast<size_t>(length)]);
    MessageHeader* header = get_message_header(message);
    header->length = static_cast<short>(length);
    return message;
}

MessageHeader* DAIDE::get_message_header(const MessagePtr &message) {
    return reinterpret_cast<MessageHeader*>(message.get());
}
