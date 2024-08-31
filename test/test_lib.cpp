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

void Concat(char const* s1, int size1, char const* s2, int size2, char* out) {
    memcpy(out, s1, size1);
    memcpy(out+size1, s2, size2);
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
