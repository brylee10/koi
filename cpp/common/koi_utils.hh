#pragma once
#include "spdlog/cfg/env.h"

// MacOS M1 Max cache line size is 128 bytes
constexpr size_t CACHE_LINE_BYTES = 128;

void load_spdlog_level();

template <typename T>
constexpr std::size_t size_rounded_to_cache_line()
{
    return (sizeof(T) + CACHE_LINE_BYTES - 1) & ~(CACHE_LINE_BYTES - 1);
}

// Returns the smallest number greater or equal to the sizeof(T) which is CACHE_LINE_BYTES * N
// where N is a power of 2
constexpr std::size_t size_rounded_up_to_pow_2_cache_line(size_t s)
{
    // While not a multiple of a cache line or not a power of two
    while (s & (CACHE_LINE_BYTES - 1) || s & (s - 1))
    {
        s++;
    }
    return s;
}