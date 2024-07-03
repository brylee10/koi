#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define COMPILE_ASSERTS 1

#ifdef COMPILE_ASSERTS
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr) ((void)0)
#endif

void report_and_exit(const char *msg);
