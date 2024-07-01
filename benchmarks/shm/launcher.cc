#include "args.hh"

#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    Args args = parse_args(argc, argv);
    pid_t server_pid, client_pid;

    // Convert integers to strings
    std::string message_size_str = std::to_string(args.message_size);
    std::string iterations_str = std::to_string(args.iterations);

    // Convert strings to const char*
    const char *message_size = message_size_str.c_str();
    const char *iterations = iterations_str.c_str();

    std::cout << "Message size and iterations: " << message_size << " " << iterations << std::endl;
    (void)message_size;
    (void)iterations;

    // Fork to create the server process
    server_pid = fork();
    if (server_pid < 0)
    {
        std::cerr << "Failed to fork server process" << std::endl;
        perror("fork");
        return 1;
    }
    else if (server_pid == 0)
    {
        // Child process for server
        execl("bin/shm/server", "server", "-m", message_size, "-i", iterations, (char *)NULL);
        perror("execl");
        // If execl returns, it means there was an error
        std::cerr << "Failed to execute server process" << std::endl;
        return 1;
    }

    // Sleep for a bit to allow the server to start
    sleep(1);

    // Fork to create the client process
    client_pid = fork();
    if (client_pid < 0)
    {
        std::cerr << "Failed to fork client process" << std::endl;
        return 1;
    }
    else if (client_pid == 0)
    {
        // Child process for client
        execl("bin/shm/client", "client", "-m", message_size, "-i", iterations, (char *)NULL);
        perror("execl");
        // If execl returns, it means there was an error
        std::cerr << "Failed to execute client process" << std::endl;
        return 1;
    }

    // Parent process
    std::cout << "Server PID: " << server_pid << std::endl;
    std::cout << "Client PID: " << client_pid << std::endl;

    // Wait for server and client to finish
    int status;
    waitpid(server_pid, &status, 0);
    waitpid(client_pid, &status, 0);

    return 0;
}