#define EXPORT __declspec(dllexport)

typedef void (*TCallback)(int val);

extern "C" {
// Simple function with only a single input value and a simple return
EXPORT bool Invert(bool input);

// Takes two input strings and returns the concatenation
EXPORT void Concat(char const* s1, int size1, char const* s2, int size2, char* out);

// Passing a callback to the DLL. This will be called 5 times with an incrementing index as the argument by the DLL.
EXPORT void SetCallback(TCallback cb);
}
