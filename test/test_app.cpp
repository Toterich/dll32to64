#include <cassert>

#include "test_lib.h"

int main() {
    assert(!Invert(true));
    assert(Invert(false));
}
