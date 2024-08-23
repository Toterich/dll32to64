#include "socket.h"

#include <plog/Log.h>

namespace sock {

bool StartupWinSock()
{
    PLOG_INFO << "Starting WinSock";

    WSADATA data;
    WORD const ver = MAKEWORD(2,2);  // 2.2 is highest available WinSock version as of 23.08.2024
    int const wsResult = WSAStartup(ver, &data);
    if (wsResult != 0)
    {
        PLOG_ERROR << "WSAStartup() Error: " << wsResult;
        return false;
    }

    return true;
}

bool CreateSocket(SOCKET &sock)
{
    PLOG_INFO << "Creating new Socket";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        int const lastError = WSAGetLastError();
        PLOG_ERROR << "socket() Error: " << lastError;
        return false;
    }

    return true;
}

bool Send(SOCKET socket, char const *buf, int size)
{
    int totalBytesSent = 0;

    // send() may return before the whole buffer has been sent
    while (totalBytesSent < size)
    {
        int const bytesSent = send(socket, &buf[totalBytesSent], size - totalBytesSent, 0);
        if (bytesSent == SOCKET_ERROR)
        {
            PLOG_ERROR << "send() Error: " << bytesSent;
            return false;
        }

        totalBytesSent += bytesSent;
    }

    return true;
}

bool Receive(SOCKET socket, char *buf, int bufSize, int& recvBytes)
{
    recvBytes = recv(socket, buf, bufSize, 0);
    if (recvBytes == SOCKET_ERROR)
    {
        int const lastError = WSAGetLastError();
        PLOG_ERROR << "recv() Error: " << lastError;
        return false;
    }

    if (recvBytes == 0)
    {
        PLOG_WARNING << "Connection closed";
        return false;
    }

    return true;
}

} // end namespace
