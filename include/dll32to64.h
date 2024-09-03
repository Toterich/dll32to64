#ifndef DLL32TO64_H
#define DLL32TO64_H

#define EXPORT __declspec(dllexport)

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
    EXPORT bool Dll32To64_EnableLogging(char const *path);

    /**
     * Shutdown the Wrapper executable.
     */
    EXPORT void Dll32To64_Shutdown();
}

#endif