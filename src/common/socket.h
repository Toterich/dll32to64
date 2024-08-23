#ifndef DLL32TO64_SOCKET_H
#define DLL32TO64_SOCKET_H

// Need to define the Windows version manually if not using MSVC
#ifndef NTDDI_VERSION
    #define NTDDI_VERSION NTDDI_WIN10_19H1
#endif
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#include <WS2tcpip.h>  // Windows Sockets

namespace sock {

// Establish connections on loopback address
char const ipAddress[] = "127.0.0.1";
// Port for Request-Response connection
int const requestPort = 54000;
// Port for Callback connection
int const callbackPort = 54001;

// Wrapper around WSAStartup()
bool StartupWinSock();

/* Create a socket. */
bool CreateSocket(SOCKET &sock);

/* Wrapper around send() syscall, that only returns once all of buf has been sent (or an error occured) */
bool Send(SOCKET socket, char const *buf, int size);

/*
 * Wrapper around recv() syscall, that handles receive errors
 *
 * If this returns false, 'recvBytes' will contain the error code instead of the number of received bytes.
 */
bool Receive(SOCKET socket, char *buf, int bufSize, int& recvBytes);

} // end namespace

#endif // DLL32TO64_SOCKET_H
