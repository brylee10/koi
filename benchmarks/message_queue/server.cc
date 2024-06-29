#include "signals.hh"
#include "bench.hh"
#include "message.hh"
#include "mq.hh"

#include <iostream>
// Prefer System V message queues over POSIX message queues on MacOS
// https://stackoverflow.com/questions/10079403/cannot-find-include-file-mqueue-h-on-os-x
#include <sys/msg.h>
#include <cstdio>

constexpr char QUEUE_NAME[] = "/test_queue";
constexpr key_t QUEUE_KEY = 0x1234;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <message_size> <iterations>\n";
        return 1;
    }

    ull message_size = std::strtoul(argv[1], nullptr, 10);
    ull iterations = std::strtoul(argv[2], nullptr, 10);

    if (message_size == 0 || iterations == 0 || message_size > MAX_MSG_SIZE)
    {
        std::cerr << "Both message_size and iterations must be positive integers and message_size must be less than "
                  << MAX_MSG_SIZE << "\n";
        return 1;
    }

    std::cout << "Message size: " << message_size << " bytes\n"
              << "Iterations: " << iterations << "\n";

    int msq_id = msgget(QUEUE_KEY, IPC_CREAT);

    if (msq_id == -1)
    {
        perror("msgget");
        return 1;
    }

    return 0;
}