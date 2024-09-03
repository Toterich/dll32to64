# dll32to64
A proof of concept to create a wrapper around a 32bit Windows DLL to be able to load it in a 64bit application.

## Why?

If you find yourself in the situation where you need to integrate a 3rd party library into your application, but said library is compiled for a different architecture than your client program and you don't have access to the source, this might be for you.

## Current State and plans

This is currently only a proof of concept which can't be used for any real projects out of the box. It is hardcoded to bridge the library in `test/test_lib.cpp` to the application in `test/test_app.cpp`.

In order to apply to your own code, the code in `src` needs to be adapted in several places (grep for the comment `TODO: AUTOGEN`).

I am planning to add a code generator that creates the c++ code to wrap an arbitrary dll from a DSL, at which point it could serve as a tool to wrap arbitrary libraries.

## Functional Overview

Building this project creates two binaries:

* A Bridge DLL, which exports the same symbol names as the DLL to be wrapped. This is intended to be used as a drop-in replacement for the wrapped DLL.
* A Wrapper executable, which links against the wrapped DLL. This communicates with the Bridge DLL via a TCP socket connection.

Any calls of exported DLL functions made by the client program are then serialized using a custom binary protocol and sent to the wrapper executable via the socket connection. The wrapper deserializes the message and calls the actual function of the wrapped DLL.

The following diagram illustrates the data flow from a 64bit client application to a 32bit library:

```
                              64bit || 32bit
                                    ||
+-------------------+               ||    +-------------------------+
|                   |               ||    |  wrapped.dll            |
|      Client       |               ||    |                         |
|                   |               ||    +-------------------------+
+-------------------+               ||             /\
        |                           ||             |
        |                           ||             |
        |                           ||             |
        | links DLL                 ||             | links DLL
        |                           ||             |
        \/                          ||             |
+=======================+  Req/Res  ||       +=========================+
|     bridge.dll        |<------------------>|  wrapper.exe            |
|                       |           ||       |                         |
|                       |<-------------------|                         |
|                       |  Callback ||       |                         |
+=======================+           ||       +=========================+
```

## Dependencies

This project uses the `MinGW` compiler toolchain. Additionally, `Python3` is required to execute the build script.

The build has been successfully tested on the [MSYS2](https://www.msys2.org/) environment. Once installed, open an Msys terminal and run the following

```bash
pacman -Syu
pacman -Sy mingw-w64-x86_64-toolchain mingw-w64-i686-toolchain python3
```

## Config and Build

```bash
python3 build.py
```

compiles `bridge.dll` and `wrapper.exe`. Try

```bash
python3 build.py --help
```

to see a list of parameters. All of these can also be specified in a config file. For this, copy `build_params.py.template` to `build_params.py` and modify the values accordingly.

## Running the testsuite

From the Msys shell, do 

```bash
python3 test/build_test.py
```

to build all the required binaries and execute the test application. This builds `test_lib.dll` with the configured 32bit compiler and `test_app.exe` with the 64bit compiler, as well as `bridge.dll` in 64bit and `wrapper.exe` in 32bit. `test_app` links to `bridge.dll` and `wrapper.exe` to `test_lib.dll`, so exported function calls of `test_lib` can be tunneled to `test_app`.