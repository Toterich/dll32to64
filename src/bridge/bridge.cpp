#include "common/common.h"

#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include <cstring>
#include <cstdio>
#include <mutex>
#include <thread>
#include <string>
#include <sstream>

// TODO: AUTOGEN
#include "test_lib.h"

// Additional exported functions that are not part of wrapped DLL
extern "C"
{
    // Max stringlength of the `path` argument to EnableLogging
    unsigned const LOG_DIR_MAXLEN = 2048;

    /**
     * Start logging calls to bridge.dll
     *
     * @param path: 0-terminated path to directory where log files should be stored.
     * @return True if logging was started successfully.
     */
    bool EnableLogging(char const *path);

    /**
     * Shutdown the Wrapper executable.
     */
    void Shutdown();
}

namespace {

bool winSockStartup = false;

// Socket to maintain request-response connection
SOCKET requestSocket = INVALID_SOCKET;
// Socket to maintain callback connection
SOCKET callbackSocket = INVALID_SOCKET;
// Handle to wrapper.exe once it was launched
HANDLE wrapperProcess = INVALID_HANDLE_VALUE;

// User defined callback functions
// TODO: AUTOGEN
TCallback callback = NULL;

// Thread executing CallbackTask
std::thread callbackThread;

bool ConnectToWrapper(SOCKET &socket, int port)
{
    PLOG_INFO << "Establishing Socket connection to wrapper on port " << port;

    if (INVALID_SOCKET != socket)
    {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }

    if (!sock::CreateSocket(socket))
    {
        return false;
    }

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, sock::ipAddress, &hint.sin_addr);

	int connResult = connect(socket, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
        PLOG_ERROR << "Can't connect to wrapper exe on port " << port << ", Err: " << WSAGetLastError();
		closesocket(socket);
        socket = INVALID_SOCKET;
		return false;
	}

    return true;
}

/* Connect to Wrapper via callbackPort and wait for forwarded Callback executions. */
void CallbackTask()
{
    PLOG_INFO << "Starting Callback Thread";

    if (!sock::StartupWinSock())
    {
        PLOG_WARNING << "Exiting Callback Thread due to previous error";
        return;
    }

    if (!ConnectToWrapper(callbackSocket, sock::callbackPort))
    {
        WSACleanup();
        PLOG_WARNING << "Exiting Callback Thread due to previous error";
        return;
    }

    char incoming[msg::MSG_MAX_SIZE];
    while (true)
    {
        int recvBytes;
        if (!sock::Receive(callbackSocket, incoming, sizeof(incoming), recvBytes))
        {
            PLOG_INFO << "Stop waiting for callbacks because connection was closed";
            break;
        }

        msg::MessageData message;
        if (!msg::ParseMessage(message, msg::DIRECTION_Response, incoming, sizeof(incoming)))
        {
            continue;
        }

        PLOG_DEBUG << "Callback " << message.id;

        switch (message.id)
        {
            case msg::MSGID_Callback:
            {
                if (callback != NULL)
                {
                    callback(message.staticData.CallbackResponse.val);
                }
            } break;
            default:
                break;
        }
    }

    if (callbackSocket != INVALID_SOCKET)
    {
        closesocket(callbackSocket);
        callbackSocket = INVALID_SOCKET;
    }

    WSACleanup();
}

bool EnsureWrapperConnection()
{
    // This function accesses/modifies some persistent state so we only allow execution of it
    // by a single thread at a time
    static std::mutex mut;
    std::lock_guard<std::mutex> guard(mut);

    if (!winSockStartup)
    {
        // We need to do this once after the DLL was loaded
        if (!sock::StartupWinSock())
        {
            return false;
        }
        winSockStartup = true;
    }

    // Check if wrapper exe is already running
    bool wasRunning = false;
    if (wrapperProcess != INVALID_HANDLE_VALUE)
    {
        DWORD exitCode;
        if (!GetExitCodeProcess(wrapperProcess, &exitCode))
        {
            int const lastError = GetLastError();
            PLOG_ERROR << "GetExitCodeProcess() Error: " << lastError;
            return false;
        }

        if (STILL_ACTIVE == exitCode)
        {
            wasRunning = true;
        }
        else
        {
            PLOG_INFO << "Wrapper exited with exitcode " << exitCode;
            WaitForSingleObject(wrapperProcess, INFINITE);
            CloseHandle(wrapperProcess);
            wrapperProcess = INVALID_HANDLE_VALUE;
        }
    }

    if (!wasRunning)
    {
        PLOG_INFO << "Starting Wrapper";

        // First, get full path to directory where the DLL lies
        // See https://stackoverflow.com/a/6924332
        HMODULE hm = NULL;
        // Needs the name of one exported function of the DLL
        // TODO: AUTOGEN
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)&Invert, &hm))
        {
            int const lastError = GetLastError();
            PLOG_ERROR << "GetModuleHandleEx() Error: " << lastError;
            return false;
        }
        static char path[4096];
        if (!GetModuleFileNameA(hm, path, sizeof(path)))
        {
            int const lastError = GetLastError();
            PLOG_ERROR << "GetModuleFileName() Error: " << lastError;
            return false;
        }

        // Remove Filename from path
        std::string const pathS(path);
        int const lastSlashPos = pathS.find_last_of("\\/");

        // Append exe name to path
        // TODO: Respect different filename depending on wrapping direction
        std::strcpy(&path[lastSlashPos + 1], "dll32to64_wrapper.exe");

        PLOG_DEBUG << "Running " << path;

        STARTUPINFOA si = {};
        PROCESS_INFORMATION pi = {};

        if (!CreateProcessA(path,
                NULL,
                NULL,
                NULL,
                false,
                0,
                NULL,
                NULL,
                &si,
                &pi
            )
        )
        {
            int const lastError = GetLastError();
            PLOG_ERROR << "CreateProcess() Error: " << lastError;
            return false;
        }

        wrapperProcess = pi.hProcess;
        CloseHandle(pi.hThread);  // Don't need this handle
    }

    // (Re)connect to wrapper if it was just started or we don't have a socket handle yet
    if (!wasRunning || requestSocket == INVALID_SOCKET)
    {
        if (!ConnectToWrapper(requestSocket, sock::requestPort))
        {
            return false;
        }
    }

    // Same for callback connection
    if (!wasRunning || callbackSocket == INVALID_SOCKET)
    {
        if (callbackThread.joinable())
        {
            PLOG_INFO << "Joining Callback Thread";
            callbackThread.join();
        }

        callbackThread = std::thread(CallbackTask);
    }

    return true;
}

bool SendAndWaitForResponse(msg::MessageData const &message, msg::MessageData &response)
{
    static char messageBuffer[msg::MSG_MAX_SIZE];
    static char responseBuffer[msg::MSG_MAX_SIZE];

    // Ensure that only one thread at a time can send a message and wait for its response
    static std::mutex mut;
    std::lock_guard<std::mutex> guard(mut);

    PLOG_DEBUG << "Sending Messsage " << message.id;

    int messageSize;
    msg::SerializeMessage(message, messageBuffer, messageSize);
    if (!sock::Send(requestSocket, messageBuffer, messageSize))
    {
        return false;
    }

    while (true)
    {
        int recvBytes;
        if (!sock::Receive(requestSocket, responseBuffer, sizeof(responseBuffer), recvBytes))
        {
            return false;
        }

        if (!msg::ParseMessage(response, msg::DIRECTION_Response, responseBuffer, sizeof(responseBuffer)))
        {
            return false;
        }

        PLOG_DEBUG << "Received Response " << response.id;

        if (response.id != message.id)
        {
            PLOG_ERROR <<  "Waiting for MsgId " << message.id << ", but received " << response.id;
            return false;
        }

        return true;
    }
}

template <typename T>
std::string StringifyArray(const T* arr, size_t num)
{
    std::ostringstream stream(std::ios_base::ate | std::ios_base::out);

    stream << "[";

    for (unsigned i = 0; i < num; i++)
    {
        stream << arr[i] << ", ";
    }

    stream << "]";

    return stream.str();
}

template <size_t N>
std::string StringifyArrayOfStrings(const char (*arr)[N], size_t num)
{
    std::ostringstream stream(std::ios_base::ate | std::ios_base::out);

    stream << "[";

    for (unsigned i = 0; i < num; i++)
    {
        stream << std::string(arr[i], strnlen(arr[i], N)) << ", ";
    }

    stream << "]";

    return stream.str();
}

} // end anonymous namespace

bool EnableLogging(char const *path)
{
    if (path == nullptr)
    {
        return false;
    }

    unsigned const pathLen = strnlen(path, LOG_DIR_MAXLEN);

    if (LOG_DIR_MAXLEN == pathLen)
    {
        // Path is not 0-terminated
        return false;
    }

    char const logFileName[] = "/dll32to64.log";

    char fullPath[LOG_DIR_MAXLEN + sizeof(logFileName)];
    strcpy(fullPath, path);

    // Append logfile name to path
    std::strcpy(&fullPath[pathLen], logFileName);

#if DEBUG == 1
    plog::Severity const severity = plog::debug;
#else
    plog::Severity const severity = plog::info;
#endif

    // Rotate up to 10 logfiles of 10MB each
    plog::init(severity, fullPath, 10000000, 10);

    printf("dll32to64 Logfile: %s\n", fullPath);
    return true;
}

void Shutdown()
{
    PLOG_INFO << "Shutdown";

    // This will initiate shutdown in wrapper.exe
    if (requestSocket != INVALID_SOCKET) closesocket(requestSocket);

    // Wait until wrapper has shut down (peer will close callback connection)
    if (wrapperProcess != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(wrapperProcess, INFINITE);
        CloseHandle(wrapperProcess);
    }

    // Callback thread should have exited know and join immediately
    if (callbackThread.joinable()) callbackThread.join();

    WSACleanup();
}


// wrapped dll implementation
// TODO: AUTOGEN

bool Invert(bool input) {
    if (!EnsureWrapperConnection()) return false;

    msg::MessageData message = {};
    msg::InitMessageData(message, msg::MSGID_Invert, msg::DIRECTION_Request);
    message.staticData.Invert.input = input;

    msg::MessageData response = {};
    if (!SendAndWaitForResponse(message, response)) return false;
    return response.staticData.InvertResponse;
}

void Concat(char const* s1, int size1, char const* s2, int size2, char* out) {
    if (!EnsureWrapperConnection()) return;

    msg::MessageData message = {};
    msg::InitMessageData(message, msg::MSGID_Invert, msg::DIRECTION_Request);
    message.staticData.Concat.s1.byte_offset = 0;
    message.staticData.Concat.s1.byte_length = size1;
    message.staticData.Concat.s2.byte_offset = size1;
    message.staticData.Concat.s2.byte_length = size2;

    msg::MessageData response = {};
    if (!SendAndWaitForResponse(message, response)) return;

    if (message.variableDataLength > sizeof(message.variableData))
    {
        PLOG_ERROR << "Data length exceeded (" << message.variableDataLength << ">" << sizeof(message.variableData) << ")";
        return;
    }

    std::memcpy(message.variableData, s1, size1);
    std::memcpy(message.variableData + size1, s2, size2);

    msg::MessageData responseMessage = {};
    if (!SendAndWaitForResponse(message, responseMessage)) return;

    size_t const responseVdSize = responseMessage.staticData.ConcatResponse.out.byte_length;
    if (responseVdSize > sizeof(responseMessage.variableData))
    {
        PLOG_ERROR << "Response data length exceeded (" << responseVdSize << ">" << sizeof(responseMessage.variableData) << ")";
        return;
    }

    std::memcpy(out,
                &responseMessage.variableData[responseMessage.staticData.ConcatResponse.out.byte_offset],
                responseVdSize);
}

void SetCallback(TCallback cb)
{
    if (!EnsureWrapperConnection()) return;

    // Store callback pointer before sending message, in case the first callback arrives before SetCallback's response
    // arrives
    callback = cb;

    msg::MessageData message = {};
    msg::InitMessageData(message, msg::MSGID_Callback, msg::DIRECTION_Request);

    msg::MessageData response = {};
    if (!SendAndWaitForResponse(message, response)) return;
}
