# dll32to64
A proof of concept to create a wrapper around a 32bit Windows DLL to be able to load it in a 64bit application, or vice versa.

## Why?

If you find yourself in the situation where you need to integrate a 3rd party library into your application, but said library is compiled for a different architecture than your client program and you don't have access to the source, this might be for you.

## Functional Overview

Building this project created two binaries:

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

## Current State and plans

This is currently only a proof of concept which can't be used for any real problems out of the box. It is hardcoded to bridge the library in `test/test_lib.cpp` to the application in `test/test_app.cpp`.

In order to apply to your own code, the code in `src` needs to be adapted in several places.

I am planning to extend this project with some kind of DSL to autogenerate all the library-specific portions of the code, at which point it could serve as a tool to wrap arbitrary libraries.
