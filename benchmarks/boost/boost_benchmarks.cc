#include "receiver.hh"
#include "sender.hh"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <benchmark/benchmark.h>
#include <cassert>
#include <iostream>

template <size_t message_size>
struct Message
{
    char data[message_size];
};

template <size_t message_size>
std::ostream &operator<<(std::ostream &os, Message<message_size> &msg)
{
    os << "Message: ";
    for (size_t i = 0; i < message_size; ++i)
    {
        os << static_cast<int>(msg.data[i]) << " ";
    }
    return os;
}

// `queue_size` is the total bytes of the shared memory segment. This means the number of elements
// that can be held is the floor of `queue_size / message_size`.
template <size_t queue_size, size_t message_size>
void BM_PingPong(benchmark::State &state)
{
    // Randomly generate a name for the queue
    const std::string name = "test" + std::to_string(rand());
    Sender sender = Sender<Message<message_size>>(queue_size, name);
    Receiver receiver = Receiver<Message<message_size>>(name);
    spdlog::info("Running ping-pong benchmark with message size: {}, queue size: {}, and test name {}", message_size, queue_size, name);
    spdlog::info("Starting sender free memory: {}", sender.get_free_memory());
    Message<message_size> msg = {};
    // Assign random values to the message
    for (size_t i = 0; i < message_size; i++)
    {
        msg.data[i] = rand() % 256;
    }
    // std::cout << msg << std::endl;
    for (auto _ : state)
    {
        sender.send(msg);
        // Confirm the receiver received correct messages
        std::optional<Message<message_size>> received_msg = receiver.recv();
        assert(received_msg.has_value());
        Message received = received_msg.value();
        for (size_t i = 0; i < message_size; i++)
        {
            assert(received.data[i] == msg.data[i]);
        }
    }
}

// Boost Deque SHM benchmarks
BENCHMARK(BM_PingPong<10000, 128>);

// Run the benchmark
BENCHMARK_MAIN();