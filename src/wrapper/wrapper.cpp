/**
 * Upon startup, this program runs a listener socket and waits to receive messages of the format defined
 * in msg_protocol.h.
 * When a message is received, its contents are parsed and the corresponding function of the wrapped DLL is executed. The
 * calls response is then returned via the socket.
 */

#include "common/common.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <mutex>

namespace {

// Socket to maintain request-response connection
SOCKET requestSocket = INVALID_SOCKET;
// Socket to maintain callback connection
SOCKET callbackSocket = INVALID_SOCKET;

/*
 * Mutex for locking the callback Socket.
 *
 * We don't know multiple callbacks may be executed simultaneously by different threads inside the wrapped DLL,
 * so to be safe we lock access to the resources shared by all callbacks (message buffer + socket).
 */
std::mutex callbackMutex;

void SerializeAndSendCallbackResponse(msg::MessageData const &message)
{
    static char buf[msg::MSG_MAX_SIZE];
    int messageSize;

    DBG_LOG("WRAPPER: Send Callback %d\n", message.id);
    std::lock_guard<std::mutex> guard(callbackMutex);
    msg::SerializeMessage(message, buf, messageSize);
    sock::Send(callbackSocket, buf, messageSize);
}

// See TCallback, will be called by a separate thread from inside the wrapped DLL
void Callback(int val)
{
    if (callbackSocket == INVALID_SOCKET)
    {
        return;
    }

    msg::MessageData message;
    msg::InitMessageData(message, msg::MSGID_Callback, msg::DIRECTION_Response);
    message.staticData.CallbackResponse.val = val;

    SerializeAndSendCallbackResponse(message);
}

/* Listen on the specified port for an incoming connection from Bridge. */
SOCKET WaitForClient(int port)
{
    SOCKET listener;
    if (!sock::CreateSocket(listener))
    {
        return INVALID_SOCKET;
    }

    // Bind socket to given port on loopback address
    sockaddr_in addressHint;
    addressHint.sin_family = AF_INET;
    addressHint.sin_port = htons(port);
    inet_pton(AF_INET, sock::ipAddress, &addressHint.sin_addr);

    int const bindResult = bind(listener, (sockaddr*)&addressHint, sizeof(addressHint));
    if (bindResult != 0)
    {
        printf("WRAPPER: bind() Error: %d\n", bindResult);
        closesocket(listener);
        return INVALID_SOCKET;
    }

    // Only accept a single connection
    listen(listener, 1);
    DBG_LOG("WRAPPER: Listening for client on port %d...\n", port);

    sockaddr_in client;
    int clientSize = sizeof(client);
    SOCKET clientSocket = accept(listener, (sockaddr*)&client, &clientSize);
    if (clientSocket == INVALID_SOCKET)
    {
        int const lastError = WSAGetLastError();
        printf("WRAPPER: accept() Error: %d\n", lastError);
        closesocket(listener);
        return INVALID_SOCKET;
    }

    DBG_LOG("WRAPPER: Client on port %d accepted. Waiting for messages...\n", port);

    // Listening socket can be closed after connection has been established
    closesocket(listener);

    return clientSocket;
}

int Shutdown(int exitArg)
{
    if (callbackSocket != INVALID_SOCKET) closesocket(callbackSocket);
    if (requestSocket != INVALID_SOCKET) closesocket(requestSocket);
    WSACleanup();
    return exitArg;
}

} // end anonymous namespace

// Main entry point
int main()
{
    if (!sock::StartupWinSock())
    {
        return 1;
    }

    // Wait for connection from Bridge DLL that will send requests and receive responses
    requestSocket = WaitForClient(sock::requestPort);
    if (requestSocket == INVALID_SOCKET)
    {
        printf("WRAPPER: Could not connect to Message Client\n");
        return Shutdown(2);
    }

    // Wait for connection from Bridge DLL that will receive callbacks
    callbackSocket = WaitForClient(sock::callbackPort);
    if (callbackSocket == INVALID_SOCKET)
    {
        printf("WRAPPER: Could not connect to Callback Client\n");
        return Shutdown(3);
    }

    char incoming[msg::MSG_MAX_SIZE];

    // Wait for incoming requests and respond to them
    while (true)
    {
        int recvBytes;
        if (!sock::Receive(requestSocket, incoming, sizeof(incoming), recvBytes))
        {
            printf("WRAPPER: Receive() Error: %d\n", recvBytes);
            return Shutdown(recvBytes);
        }

        msg::MessageData message = {};
        if (!ParseMessage(message, msg::DIRECTION_Request, incoming, recvBytes))
        {
            printf("WRAPPER: ParseMessage() Error (recvBytes: %d)\n", recvBytes);
            continue;
        }

        // Call requested function and craft response
        // TODO: Autogenerate

        msg::MessageData response = {};
        InitMessageData(response, message.id, msg::DIRECTION_Response);

        switch (message.id)
        {
            case msg::MSGID_Callback: // fall-through
                printf("WRAPPER: Received unexpected MsgId: %d. This is ignored.\n", message.id);
                continue;
            case msg::MSGID_Invert:
            {
                response.staticData.InvertResponse = Invert(message.staticData.Invert.input);
            } break;
            case msg::MSGID_Concat:
            {
                char* const s1 = &message.variableData[message.staticData.Concat.s1.byte_offset];
                int size1 = message.staticData.Concat.s1.byte_length;
                char* const s2 = &message.variableData[message.staticData.Concat.s2.byte_offset];
                int size2 = message.staticData.Concat.s2.byte_length;
                char output[msg::MSG_MAX_SIZE];
                Concat(s1, size1, s2, size2, output);

                int const outputLength = strnlen(output, msg::MSG_MAX_SIZE - 1) + 1;

                response.staticData.ConcatResponse.out.byte_offset = 0;
                response.staticData.ConcatResponse.out.byte_length = outputLength;

                std::memcpy(response.variableData, output, outputLength);
                response.variableDataLength = outputLength;
            } break;
            default: assert(false);
        }

        DBG_LOG("WRAPPER: Sending response for message %d\n", message.id);
        static char buf[msg::MSG_MAX_SIZE];
        int responseSize;
        msg::SerializeMessage(response, buf, responseSize);
        // TODO: Handle Send error
        sock::Send(requestSocket, buf, responseSize);
    }

    return Shutdown(0);
}
