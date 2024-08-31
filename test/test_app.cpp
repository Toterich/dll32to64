#include <windows.h>

#include <cassert>
#include <cstring>
#include <vector>

#include "test_lib.h"

namespace {
    std::vector<int> cbVals;

    void TestCallback(int v) {
        cbVals.push_back(v);
    }
}

int main() {
    assert(!Invert(true));
    assert(Invert(false));

    char s1[] = "First String";
    char s2[] = "Second String";
    char concatenated[512];
    Concat(s1, sizeof(s1), s2, sizeof(s2), concatenated);
    assert(0 == strcmp(concatenated, "First String\nSecond String"));

    SetCallback(TestCallback);
    std::vector<int> expected{0, 1, 2, 3, 4};
    assert(cbVals == expected);

    return 0;
}
