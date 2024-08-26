#include "receiver.hh"
#include <spdlog/spdlog.h>
#include <benchmark/benchmark.h>

template <size_t message_size>
struct Message
{
    char data[message_size];
};

template <size_t queue_size, size_t message_size>
void BM_SingleThread_Empty_PingPong(benchmark::State &state)
{
    // spdlog::set_level(spdlog::level::err);

    // // Randomly generate a name for the queue
    // const std::string name = "test" + std::to_string(rand());
    // Sender sender = Sender<Message<message_size>>(queue_size, name);
    // Receiver receiver = Receiver<Message<message_size>>(name);
    // spdlog::info("Running ping-pong benchmark with message size: {}, queue size: {}, and test name {}", message_size, queue_size, name);
    // Message<message_size> msg = {};
    // // Assign 1 to all values in the message
    // for (size_t i = 0; i < message_size; i++)
    // {
    //     msg.data[i] = 1;
    // }
    // for (auto _ : state)
    // {
    //     sender.send(msg);
    //     // Confirm the receiver received correct messages
    //     Message<message_size> received = receiver.recv();
    //     for (size_t i = 0; i < message_size; i++)
    //     {
    //         // Disabled in release mode
    //         assert(received.data[i] == msg.data[i]);
    //     }
    // }
}

// Boost Deque SHM benchmarks
BENCHMARK(BM_SingleThread_Empty_PingPong<1 << 20, 1 << 2>);

// Run the benchmark
BENCHMARK_MAIN();