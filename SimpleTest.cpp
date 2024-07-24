#include "VirtualMemory.h"

#include <cstdio>
#include <cassert>

int main(int argc, char **argv)
{
    VMinitialize();
    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i)
    {
        printf("writing to %llu\n", (long long int)i);
        VMwrite(1 * i * 1, i);
    }

    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i)
    {
        word_t value;
        VMread(1 * i * 1, &value);
        printf("reading from %llu %d\n", (long long int)i, value);
        assert(uint64_t(value) == i);
    }
    printf("success\n");

    return 0;
}
