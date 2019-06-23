// JPN_Socket.h: interface for the JPN::Socket class and related material.

// Copyright (C) 2012, John Newbury. See "Conditions of Use" in johnnewbury.co.cc/diplomacy/conditions-of-use.htm.

// Release 8~2~b

/////////////////////////////////////////////////////////////////////////////

#ifndef _JPN_SOCKET
#define _JPN_SOCKET

namespace JPN {

/////////////////////////////////////////////////////////////////////////////

#define LITTLE_ENDIAN 1 // NZ for platforms that use little endian ordering

/////////////////////////////////////////////////////////////////////////////

class MessageHeader {
    // Message header, common to all types.
public:
    char type; // type of message
    char pad; // unused; maintains 16-bit alignment
    short length; // length of body in bytes, which follows the header
};

/////////////////////////////////////////////////////////////////////////////

class Socket {
    // Message-oriented socket.

    typedef std::queue<char *> MessageQueue;

    SOCKET MySocket;

    static Socket *SocketTab[FD_SETSIZE]; // table of active Socket*
    static int SocketCnt; /// # active Socket

    MessageQueue IncomingMessageQueue; // queue of incoming messages
    MessageQueue OutgoingMessageQueue; // queue of outgoing messages
    char *IncomingMessage; // current incoming message; points to Header until length read; else a full, but incomplete, message
    char *OutgoingMessage; // current outgoing message, when partially sent; else 0
    int IncomingNext; // index of start of next incoming message in buffer
    int OutgoingNext; // index of start of next outgoingmessage in buffer
    int IncomingLength; // whole length of current incoming message, including header
    int OutgoingLength; // whole length of current outgoing message, including header
    MessageHeader Header; // buffer for header of current incoming message, pending new IncomingMessage, when length is known
    bool Connected; // true iff connected

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
    Socket() : Connected(false), IncomingMessage(0), OutgoingMessage(0) {}

    ~Socket();

    virtual bool Connect(const char *address, int port);

    char *PullIncomingMessage();

    void PushOutgoingMessage(char *message);

    void OnSocketState(LPARAM lParam);

    static Socket **FindSocket(SOCKET socket);

    static void OnSocketState(WPARAM wParam, LPARAM lParam);

    static void AdjustOrdering(short &x);

    static void AdjustOrdering(char *message, short length);
};

/////////////////////////////////////////////////////////////////////////////

} // namespace JPN

#endif
