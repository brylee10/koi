#include <chrono>
#include <iostream>
#include <algorithm> // For std::shuffle
#include <random>    // For std::default_random_engine
#include <array>

// Illustrates a way to infer the size of the cache line by measuring average
// perforamance over certain stride sizes.

int main()
{
    int array_len = 64 * 1024 * 1024;
    int *arr = new int[array_len];
    int niterations = 10;
    std::array strides = {1, 2, 4, 8, 16, 32, 64, 128, 256};

    // Obtain a time-based seed
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine engine(seed);

    // Scramble the array in place
    std::shuffle(std::begin(strides), std::end(strides), engine);

    // Reduce cold access (the first stride size is 10x worse than
    // it otherwise would be if it were not first)
    for (int i = 0; i < array_len; ++i)
    {
        arr[i] = 0;
    }

    std::array<double, strides.size()> times = {0};
    static_assert(strides.size() == times.size(), "strides and times must have the same size");
    for (int i = 0; i < niterations; ++i)
    {
        for (std::size_t j = 0; j < strides.size(); ++j)
        {
            int stride = strides[j];
            auto start = std::chrono::high_resolution_clock::now();
            // Time loop 1: increment every element
            for (int i = 0; i < array_len; i += stride)
            {
                arr[i]++;
            }
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            times[j] += elapsed.count();
        }
    }

    // Calculate average time
    for (std::size_t i = 0; i < times.size(); ++i)
        std::cout << "Time for incrementing every " << strides[i] << "th element: " << times[i] / niterations << " seconds (avg)" << std::endl;

    return 0;
}