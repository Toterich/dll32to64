#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>

#include "dll32to64.h"

#include "test_lib.h"

namespace {
    std::vector<int> cbVals;

    void TestCallback(int v) {
        cbVals.push_back(v);
    }
}

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        Dll32To64_EnableLogging(argv[1]);
    }

    assert(!Invert(true));
    assert(Invert(false));

    char s1[] = "First";
    int s1Len = strlen(s1);
    char s2[] = "Second";
    int s2Len = strlen(s2);
    char interleaved[512];
    Interleave(s1, s1Len, s2, s2Len, interleaved);
    assert(0 == memcmp(interleaved, "FSiercsotnd", s1Len + s2Len));

    SetCallback(TestCallback);
    std::vector<int> expected{0, 1, 2, 3, 4};
    assert(cbVals == expected);

    Dll32To64_Shutdown();
    printf("All tests passed successfully.\n");
    return 0;
}
