#include <iostream>
#include <vector>
#include <chrono>

// Define the size of the array
const size_t ARRAY_SIZE = 1000000;
const size_t CACHE_LINE_SIZE = 64; // Cache line size in bytes

int main()
{
    // Create a large array
    std::vector<int> largeArray(ARRAY_SIZE, 0);

    // Initialize the array
    for (size_t i = 0; i < ARRAY_SIZE; ++i)
    {
        largeArray[i] = i;
    }

    // Simulate cache miss by accessing array elements
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < ARRAY_SIZE; i += CACHE_LINE_SIZE / sizeof(int))
    {
        largeArray[i]++;
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken to access array elements: " << elapsed.count() << " seconds" << std::endl;

    return 0;
}
