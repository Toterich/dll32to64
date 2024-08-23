#ifndef DLL32TO64_COMMON_H
#define DLL32TO64_COMMON_H

#include "socket.h" // Includes WinApi headers and so should be first include
#include "msg_protocol.h"

#if defined DEBUG && DEBUG == 1
    #define DBG_LOG(x, ...) printf(x, ##__VA_ARGS__)
#else
    #define DBG_LOG(x, ...)
#endif

#endif
