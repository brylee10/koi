#include "message.hh"
#include "mq.hh"

// Reads a message from the message queue and writes it back to the server
void ping_pong(key_t mq_id)
{
    Message msg = Message<char>(10);
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

    if (message_size == 0 || iterations == 0 || message_size > MAX_MSG_SIZE)
    {
        std::cerr << "Both message_size and iterations must be positive integers and message_size must be less than "
                  << MAX_MSG_SIZE << "\n";
        return 1;
    }

    std::cout << "Message size: " << message_size << " bytes\n"
              << "Iterations: " << iterations << "\n";

    // Message queue is identified by an integer
    key_t mq_id;

    // Creates a System V message queue
    mq_id = create_mq();
    ping_pong(mq_id);

    return 0;
}