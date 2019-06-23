// JPN_Socket.cpp: implementation of the JPN::Socket class and related material.

// Copyright (C) 2012, John Newbury. See "Conditions of Use" in johnnewbury.co.cc/diplomacy/conditions-of-use.htm.

// Release 8~2~b

/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "string.h"
#include "socket.h"
#include "resource.h"
#include "error_log.h"
#include "ai_client.h"

/////////////////////////////////////////////////////////////////////////////

namespace JPN {

/////////////////////////////////////////////////////////////////////////////

Socket *Socket::SocketTab[FD_SETSIZE];

int Socket::SocketCnt;

Socket::~Socket() {
    // Destructor.

    RemoveSocket();
    delete[] OutgoingMessage;
    if (IncomingMessage != (char *) &Header) delete[] IncomingMessage;
    while (!OutgoingMessageQueue.empty()) {
        delete[] OutgoingMessageQueue.front();
        OutgoingMessageQueue.pop();
    }
    while (!IncomingMessageQueue.empty()) {
        delete[] IncomingMessageQueue.front();
        IncomingMessageQueue.pop();
    }
}

void Socket::InsertSocket() {
    // Insert `this` into SocketTab. Must not already be present; should be full, else system socket table would already be full.

    ASSERT(!FindSocket(MySocket) && SocketCnt < FD_SETSIZE);

    SocketTab[SocketCnt++] = this;
}

void Socket::RemoveSocket() {
    // Remove `this` from SocketTab iff present.

    Socket **s = FindSocket(MySocket);
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
            short length = ((MessageHeader *) OutgoingMessage)->length;
            AdjustOrdering(OutgoingMessage, length);
            OutgoingLength = sizeof(MessageHeader) + length;
        }

        int sent = send(MySocket, OutgoingMessage + OutgoingNext, OutgoingLength - OutgoingNext,
                        0); // # bytes sent, or SOCKET_ERROR

        if (sent == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                log_error("Failure %d during SendDatar", error);
                the_bot.end_dialog();
            }
            return;
        }
        OutgoingNext += sent;
        if (OutgoingNext < OutgoingLength) return; // current message not fully sent
        delete[] OutgoingMessage;
        OutgoingMessage = 0;
    }
}

void Socket::ReceiveData() {
    // Receive all data available from socket.

    ASSERT(Connected);

    const int bufferLength = 1024; // arbitrary
    char buffer[bufferLength];
    int bufferNext = 0; // index of next byte in `buffer`

    int received = recv(MySocket, buffer, bufferLength, 0);

    if (!received) {
        log_error("Failure: closed socket during read from Server");
        the_bot.end_dialog();
        return;
    }
    if (received == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            log_error("Failure %d during ReceiveData", error);
            the_bot.end_dialog();
        }
        return;
    }
    for (;;) { // while incoming data available
        int count = min(IncomingLength - IncomingNext,
                        received - bufferNext); // max # bytes that may be read into current message
        if (!count) return; // no more data available

        ASSERT(count > 0);

        memcpy(IncomingMessage + IncomingNext, buffer + bufferNext,
               count); // copy `count` bytes from `buffer` to IncomingMessage
        IncomingNext += count;
        bufferNext += count;
        if (IncomingNext == sizeof(MessageHeader)) { // just completed reading Header; length now known
            short length = ((MessageHeader *) (IncomingMessage))->length; // length of body
            AdjustOrdering(length);
            IncomingLength = sizeof(MessageHeader) + length; // full message length
            IncomingMessage = new char[IncomingLength]; // space for full message

            // Copy Header to full IncomingMessage
            ((MessageHeader *) IncomingMessage)->type = Header.type;
            ((MessageHeader *) IncomingMessage)->pad = Header.pad;
            ((MessageHeader *) IncomingMessage)->length = Header.length;

            IncomingNext = sizeof(MessageHeader);
        }
        if (IncomingNext >= IncomingLength) { // current incoming message is complete
            AdjustOrdering(IncomingMessage, IncomingLength - sizeof(MessageHeader));
            PushIncomingMessage(IncomingMessage);

            // Make IncomingMessage use Header, pending known length of next full message.
            IncomingMessage = (char *) &Header;
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

void Socket::OnConnect(int error) {
    // Handle socket Connect event; NZ `error` indicates failure.

    if (!error) Start();
    else {
        log_error("Failure %d during OnConnect", error);
        display("Failed to connect");
        the_bot.end_dialog();
    }
}

void Socket::OnClose(int error) {
    // Handle socket Close event; NZ `error` indicates failure.

    if (error) log_error("Failure %d during OnClose", error);
    the_bot.end_dialog();
}

void Socket::OnReceive(int error) {
    // Handle socket Receive event; NZ `error` indicates failure.

    if (!error) ReceiveData();
    else {
        log_error("Failure %d during OnReceive", error);
        the_bot.end_dialog();
    }
}

void Socket::OnSend(int error) {
    // Handle socket Send event; NZ `error` indicates failure.

    if (!error) SendData();
    else {
        log_error("Failure %d during OnSend", error);
        the_bot.end_dialog();
    }
}

bool Socket::Connect(const char *address, int port) {
    // Initiate asynchronous connection of socket to `address` and `port`; return true iff OK.

    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err) { // could not find a usable Winsock DLL
        log_error("Failure %d during WSAStartup %d", err);
        return false;
    }

    MySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    unsigned long mode = 1; // non-blocking
    if (ioctlsocket(MySocket, FIONBIO, &mode)) {
        log_error("Failure %d during ioctlsocket", WSAGetLastError());
        return false;
    }

    if (WSAAsyncSelect(MySocket, main_wnd, WM_SOCKET_STATE, FD_READ | FD_WRITE | FD_CONNECT | FD_CLOSE)) {
        err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            log_error("Failure %d during WSAAsyncSelect", err);
            return false;
        }
    }

    BOOL val = true;
    if (setsockopt(MySocket, SOL_SOCKET, SO_KEEPALIVE, (const char *) &val, sizeof(BOOL))) {
        log_error("Failure %d during setsockopt", WSAGetLastError());
        return false;
    }

    // Prepare `saPtr` and `saLen` args for connect, from `server` and `port`. Requires mystical incantations!
    // Assumed fast enough to do synchronously, albeit may use external name server;
    // else needs separate thread!
    sockaddr *saPtr;
    int saLen;

#define NEWSOCK WINVER >= 0x0600 // true iff using recommended newer socker functions; available in VS10 but not in VS6 

#if NEWSOCK
    // Setup the hints address info structure which is passed to the getaddrinfo() function
    ADDRINFOA hints;
    ZeroMemory(&hints, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    ADDRINFOA* ai;
    if (getaddrinfo(address, String(port), &hints, &ai)) {
        log_error("Failure %d during getaddrinfo", WSAGetLastError());
        return false;
    }

    saPtr = ai->ai_addr;
    saLen = ai->ai_addrlen;
#else // !NEWSOCK; deprecated
    sockaddr_in sa;
    ZeroMemory(&sa, sizeof sa);

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(address); // assume IP number

    if (sa.sin_addr.s_addr == INADDR_NONE) { // not valid IP number
        HOSTENT *he = gethostbyname(address); // assume IP name
        if (!he) {
            log_error("Invalid IP addres");
            return false;
        }
        sa.sin_addr.s_addr = ((IN_ADDR *) he->h_addr)->s_addr;
    }
    sa.sin_port = htons(port); // network order

    saPtr = (sockaddr * ) & sa;
    saLen = sizeof sa;
#endif

    // Connect to server.
    if (connect(MySocket, saPtr, saLen)) {
        err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            log_error("Failure %d during Connect", err);
            return false;
        }
    }

#if    NEWSOCK
    freeaddrinfo(ai); // release any space claimed by getaddrinfo
#endif

    // Make IncomingMessage use Header, pending known length of full message.
    IncomingMessage = (char *) &Header;
    IncomingNext = 0;
    IncomingLength = sizeof(MessageHeader);
    OutgoingMessage = 0;

    InsertSocket();

    return true;
}

void Socket::PushIncomingMessage(char *message) {
    // Push incoming `message` on end of IncomingMessageQueue.

    if (IncomingMessageQueue.empty()) PostMessage(main_wnd, WM_SOCKET_MESSAGE, 0, 0);
    IncomingMessageQueue.push(message); // safe to follow the post as we are in the Main thread
}

void Socket::PushOutgoingMessage(char *message) {
    // Push outgoing `message` on end of OutgoingMessageQueue.

    OutgoingMessageQueue.push(message);
    if (!OutgoingMessage && Connected) SendData();
}

char *Socket::PullIncomingMessage(void) {
    // Pull next message from front of IncomingMessageQueue. Post a new message event if more messages remain on queue.

    ASSERT(!IncomingMessageQueue.empty());

    char *incomingMessage = IncomingMessageQueue.front(); // message to be processed
    IncomingMessageQueue.pop(); // remove it

    if (!IncomingMessageQueue.empty())
        PostMessage(main_wnd, WM_SOCKET_MESSAGE, 0, 0); // trigger recall if more messages remain

    return incomingMessage;
}

void Socket::OnSocketState(LPARAM lParam) {
    // Process a change of socket state, specified by `lParam`.

    int event = WSAGETSELECTEVENT(lParam);
    int error = WSAGETSELECTERROR(lParam);

    switch (event) {
        case FD_READ:
            OnReceive(error);
            break;
        case FD_WRITE:
            OnSend(error);
            break;
        case FD_CONNECT:
            OnConnect(error);
            break;
        case FD_CLOSE:
            OnClose(error);
            break;
    }
}

Socket **Socket::FindSocket(SOCKET socket) {
    // Return the Socket** in SocketTab that owns `socket`; 0 iff not found.

    Socket **s;
    int i;
    for (i = 0; i < SocketCnt; ++i) {
        s = &SocketTab[i];
        if ((*s)->MySocket == socket) return s;
    }
    return 0;
}

void Socket::OnSocketState(WPARAM wParam, LPARAM lParam) {
    // Process change of socket state, specified by `lParam` for socket specifed by `wParam`.

    Socket **s = FindSocket((SOCKET) wParam);

    ASSERT(s);

    (*s)->OnSocketState(lParam);
}

void Socket::AdjustOrdering(short &x) {
    // Adjust the byte ordering of `x`, to or from network ordering of a `short`.

#if LITTLE_ENDIAN
    ASSERT(htons(1) != 1);

    x = (x << 8) | (((unsigned short) x) >> 8);
#else
    ASSERT(htons(1) == 1);
#endif
}

void Socket::AdjustOrdering(char *message, short length) {
    // Adjust 16-bit aligned `message`, having body `length`, to or from network ordering of its components.
    // 'length` must be specified, as that in the header may or may not be in internal order.
    // Header `type` and `pad` are single byte, so need no reordering; the rest are byte-pair `short`, so may need reordering.

#if LITTLE_ENDIAN
    AdjustOrdering(((MessageHeader *) message)->length);
    message += sizeof(MessageHeader);
    char *end = message + length;
    for (; message < end; message += sizeof(short)) AdjustOrdering(*(short *) message);
#endif
}

/////////////////////////////////////////////////////////////////////////////

} // namespace JPN