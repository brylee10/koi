#include <array>
#include <chrono>
#include <iostream>

int main()
{
    std::array<int, 2> elements = {0};
    int steps = 64 * 1024 * 1024;

    // Time loop 1: increment every element
    double total_time_exp1 = 0.0;
    auto start_exp1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < steps; i += 1)
    {
        elements[0]++;
        elements[0]++;
    }
    auto end_exp1 = std::chrono::high_resolution_clock::now();
    auto elapsed_exp1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end_exp1 - start_exp1);
    total_time_exp1 += elapsed_exp1.count();

    // Time loop 2: increment every element
    double total_time_exp2 = 0.0;
    auto start_exp2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < steps; i += 1)
    {
        elements[0]++;
        elements[1]++;
    }
    auto end_exp2 = std::chrono::high_resolution_clock::now();
    auto elapsed_exp2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end_exp2 - start_exp2);
    total_time_exp2 += elapsed_exp2.count();

    // Average time per step
    std::cout << "Avg time taken to access array (size " << elements.size() << ") elements: " << total_time_exp1 / steps << " ns" << std::endl;
    std::cout << "Avg time taken to access array (size " << elements.size() << ") elements: " << total_time_exp2 / steps << " ns" << std::endl;

    return 0;
}