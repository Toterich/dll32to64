#include <algorithm>
#include <cstring>
#include <chrono>
#include <thread>

#include "test_lib.h"

namespace {

TCallback curCb = NULL;

}

bool Invert(bool input) {
    return !input;
}

void Interleave(char const* s1, int size1, char const* s2, int size2, char* out) {
    int sMin = std::min(size1, size2);
    int outIdx = 0;
    for (int i = 0; i < sMin; i++) {
        out[outIdx] = s1[i];
        out[outIdx+1] = s2[i];
        outIdx += 2;
    }

    if (size1 == size2) {
        return;
    }

    char const* rest;
    int sMax;
    if (s1 < s2) {
        rest = &s2[sMin];
        sMax = size2;
    }
    else {
        rest = &s1[sMin];
        sMax = size1;
    }
    memcpy(out+2*sMin, rest, sMax - sMin);
}

static void CallbackTask(TCallback cb)
{
    for (int i = 0; i < 5; i++)
    {
        cb(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void SetCallback(TCallback proc)
{
    std::thread cbThread(CallbackTask, proc);
    cbThread.join();
}
