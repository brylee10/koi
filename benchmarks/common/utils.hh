#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void report_and_exit(const char *msg);

// Set to 1 to compile with runtime asserts, typically used for debugging
#define COMPILE_WITH_ASSERTS 1

#if COMPILE_WITH_ASSERTS == 1
#define ASSERT(expr)                                 \
    if (!(expr))                                     \
    {                                                \
        report_and_exit("Assertion failed: " #expr); \
    }
#else
#define ASSERT(expr) ((void)0)
#endif
