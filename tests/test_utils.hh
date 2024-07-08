#pragma once

#include <string>
#include <random>
#include <sstream>

// Platform specific
// MacOS M1 has a page size of 16KB
// `getconf PAGE_SIZE` to check
constexpr size_t PAGE_SIZE = 1 << 14;
constexpr size_t SHM_SIZE_PAGES = 1 << 1;
constexpr size_t SHM_SIZE = PAGE_SIZE * SHM_SIZE_PAGES;

// `inline` to avoid multiple definitions
inline std::string generate_unique_shm_name(std::string_view prefix = "koi_queue")
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);

    std::ostringstream oss;
    oss << prefix << dis(gen);
    return oss.str();
}