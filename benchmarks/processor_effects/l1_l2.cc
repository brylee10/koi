#include <array>
#include <chrono>
#include <iostream>

int main()
{
    // L1 cache is 132 KB, about 30k integers is needed to fill the L1 cache
    std::array<int, 36000> large_array = {0};
    // 64M steps
    int steps = 64 * 1024 * 1024;

    // // Touch each cache line (Mac cache lines are 128 bytes but this touches each 64 bytes)
    // for (int i = 0; i < 10000; i += 16)
    // {
    //     large_array[i]++;
    // }

    double total_time = 0.0;

    auto start = std::chrono::high_resolution_clock::now();
    std::size_t size_mod = large_array.size() - 1;
    for (int i = 0; i < steps; ++i)
    {
        large_array[(i * 16) & size_mod]++;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    total_time += elapsed.count();

    std::cout << "Avg time taken to access array (size " << large_array.size() << ") elements: " << total_time / steps << " seconds" << std::endl;
}