#include <thread>
#include <vector>
#include <iostream>

constexpr int NUM_THREADS = 5;

std::vector<bool> entering(NUM_THREADS);
std::vector<int> number(NUM_THREADS);

void lock(int i)
{
    entering[i] = true;
    // Does not require each `number[i]` to be unique
    int max = 0;
    for (int j = 0; j < NUM_THREADS; j++)
    {
        if (number[j] > max)
        {
            max = number[j];
        }
    }
    // Sleep to all get the same number
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (i == 0)
    {
        // Sleep 0 so it is last to get the number
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    number[i] = 1 + max;
    // std::cout << "Thread " << i << " picked number " << number[i] << std::endl;
    entering[i] = false;
    for (int j = 0; j < NUM_THREADS; j++)
    {
        while (entering[j])
        {
            // Wait while other thread picks a number
        }
        if (i != 1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        while ((number[j] != 0) && ((number[j] < number[i]) || ((number[j] == number[i]) && (j < i))))
        {
            std::cout << "Thread " << i << " waiting for thread " << j << std::endl;
            // Wait while other thread picks a number
        }
    }
}

void unlock(int i)
{
    number[i] = 0;
}

void thread(int i)
{
    while (true)
    {
        lock(i);
        // Critical section
        std::cout << "Thread " << i << " in critical section\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Simulate work
        std::cout << "Thread " << i << " leaving critical section\n";
        unlock(i);
        // Non-critical section
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    }
}

int main()
{
    std::vector<std::thread> threads;

    // Initialize Entering and Number arrays
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        entering.at(i) = false;
        number.at(i) = 0;
    }

    std::cout << "Starting " << NUM_THREADS << " threads..." << std::endl;

    // Create and start threads
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        // Thread constructed in place using `thread` and `i` as args
        threads.emplace_back(thread, NUM_THREADS - i - 1);
    }

    // Join threads (in a real application you might want to join them based on some condition)
    for (auto &t : threads)
    {
        t.join();
    }
}