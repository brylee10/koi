/*
 * This benchmark measures the time to send a message of a given size through a pipe.
 */

#include "common/utils.hh"
#include "common/bench.hh"
#include "common/signals.hh"

#include <iostream>
#include <unistd.h>
#include <string>
#include <thread>

#define READ_FD 0
#define WRITE_FD 1

void start_child(int pipefd[2], ull message_size, ull iterations)
{
    SignalManager signal_manager(SignalManager::SignalTarget::CLIENT);

    // Child process
    close(pipefd[WRITE_FD]);

    char *buffer = new char[message_size];
    for (ull i = 0; i < iterations; i++)
    {
        // Wait until server notifies client to read
        // std::cout << "Waiting for server to notify client to read\n";
        signal_manager.wait_until_notify();
        if (read(pipefd[READ_FD], buffer, message_size) < 0)
        {
            report_and_exit("read() failed");
        }
        // else
        // {
        //     std::cout << buffer << std::endl;
        // }
        // Notify server that client is done reading and tracking benchmarks
        // std::cout << "Notifying server that client is done reading\n";
        signal_manager.notify();
    }

    delete[] buffer;
    close(pipefd[READ_FD]);
}

void start_parent(int pipefd[2], ull message_size, ull iterations)
{
    // Sleep for 0.5 sec to allow child to spawn
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Benchmarks benchmarks(std::string("pipe"), message_size);
    SignalManager signal_manager(SignalManager::SignalTarget::SERVER);
    // Parent process
    close(pipefd[READ_FD]);

    char *buffer = new char[message_size];

    // Generate a string with `message_size` number of bytes
    std::string text = std::string(message_size, 'a');
    for (ull i = 0; i < iterations; i++)
    {
        if (text.copy(buffer, message_size, 0) != message_size)
        {
            report_and_exit("copy() did not copy full message size");
        }
        if (write(pipefd[WRITE_FD], buffer, message_size) < 0)
        {
            report_and_exit("write() failed");
        }
        // Notify client to read
        // std::cout << "Notifying client to read\n";
        benchmarks.startIteration();
        signal_manager.notify();
        // Wait until client is done reading and tracking benchmarks
        // std::cout << "Waiting for client to notify server that it is done reading\n";
        signal_manager.wait_until_notify();
        // Special case where each iteration is 1 message
        benchmarks.endIteration(1);
    }

    delete[] buffer;
    close(pipefd[WRITE_FD]);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <message_size> <iterations>\n";
        return 1;
    }

    ull message_size = std::strtoul(argv[1], nullptr, 10);
    ull iterations = std::strtoul(argv[2], nullptr, 10);

    if (message_size == 0 || iterations == 0)
    {
        std::cerr << "Both message_size and iterations must be positive integers\n";
        return 1;
    }

    int pipefd[2];
    if (pipe(pipefd) != 0)
    {
        report_and_exit("pipe() failed");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        report_and_exit("fork() failed");
    }

    if (pid == 0)
    {
        // Child process
        start_child(pipefd, message_size, iterations);
    }
    else
    {
        // Parent process
        start_parent(pipefd, message_size, iterations);
    }

    return 0;
}