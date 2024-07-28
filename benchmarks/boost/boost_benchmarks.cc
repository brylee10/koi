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

// Benchmarks a single thread ping pong where the queue is near empty (high contention)
// `queue_size` is the total bytes of the shared memory segment. This means the number of elements
// that can be held is the floor of `queue_size / message_size`.
template <size_t queue_size, size_t message_size>
void BM_SingleThread_Empty_PingPong(benchmark::State &state)
{
    spdlog::set_level(spdlog::level::err);

    // Randomly generate a name for the queue
    const std::string name = "test" + std::to_string(rand());
    Sender sender = Sender<Message<message_size>>(queue_size, name);
    Receiver receiver = Receiver<Message<message_size>>(name);
    spdlog::info("Running ping-pong benchmark with message size: {}, queue size: {}, and test name {}", message_size, queue_size, name);
    Message<message_size> msg = {};
    // Assign 1 to all values in the message
    for (size_t i = 0; i < message_size; i++)
    {
        msg.data[i] = 1;
    }
    for (auto _ : state)
    {
        sender.send(msg);
        // Confirm the receiver received correct messages
        Message<message_size> received = receiver.recv();
        for (size_t i = 0; i < message_size; i++)
        {
            // Disabled in release mode
            assert(received.data[i] == msg.data[i]);
        }
    }
}

template <size_t queue_size, size_t message_size>
void BM_TwoThread_Empty_PingPong(benchmark::State &state)
{
    spdlog::set_level(spdlog::level::err);

    constexpr int SENDER_THREAD_ID = 0;
    constexpr int RECEIVER_THREAD_ID = 1;

    // Randomly generate a name for the queue once
    static std::string name = "test" + std::to_string(rand());
    // Sender
    static Sender sender = Sender<Message<message_size>>(queue_size, name);
    // Receiver
    static Receiver receiver = Receiver<Message<message_size>>(name);
    if (state.thread_index() >= 2)
    {
        throw std::runtime_error("Invalid thread index. Test assumes only two threads.");
    }
    spdlog::info("Running ping-pong benchmark with message size: {}, queue size: {}, and test name {}", message_size, queue_size, name);
    Message<message_size> msg = {};
    // Assign 1 to all values in the message
    for (size_t i = 0; i < message_size; i++)
    {
        msg.data[i] = 1;
    }
    for (auto _ : state)
    {
        if (state.thread_index() == SENDER_THREAD_ID)
        {
            // If the transport is full, this will block until the receiver reads a message
            sender.send(msg);
        }
        else if (state.thread_index() == RECEIVER_THREAD_ID)
        {
            // Confirm the receiver received correct messages
            // This blocks until the sender sends a message
            Message<message_size> received = receiver.recv();
            for (size_t i = 0; i < message_size; i++)
            {
                assert(received.data[i] == msg.data[i]);
            }
        }
    }
}

// Boost Deque SHM benchmarks
BENCHMARK(BM_SingleThread_Empty_PingPong<1 << 20, 1 << 2>);
BENCHMARK(BM_SingleThread_Empty_PingPong<1 << 20, 1 << 6>);
BENCHMARK(BM_SingleThread_Empty_PingPong<1 << 20, 1 << 8>);
BENCHMARK(BM_SingleThread_Empty_PingPong<1 << 20, 1 << 12>);
BENCHMARK(BM_SingleThread_Empty_PingPong<1 << 12, 1 << 2>);
BENCHMARK(BM_SingleThread_Empty_PingPong<1 << 12, 1 << 6>);
BENCHMARK(BM_SingleThread_Empty_PingPong<1 << 12, 1 << 8>);
BENCHMARK(BM_SingleThread_Empty_PingPong<1 << 13, 1 << 12>);

BENCHMARK(BM_TwoThread_Empty_PingPong<1 << 20, 1 << 2>)->Threads(2);
BENCHMARK(BM_TwoThread_Empty_PingPong<1 << 20, 1 << 6>)->Threads(2);
BENCHMARK(BM_TwoThread_Empty_PingPong<1 << 20, 1 << 8>)->Threads(2);
BENCHMARK(BM_TwoThread_Empty_PingPong<1 << 20, 1 << 12>)->Threads(2);
BENCHMARK(BM_TwoThread_Empty_PingPong<1 << 12, 1 << 2>)->Threads(2);
BENCHMARK(BM_TwoThread_Empty_PingPong<1 << 12, 1 << 6>)->Threads(2);
BENCHMARK(BM_TwoThread_Empty_PingPong<1 << 12, 1 << 8>)->Threads(2);
BENCHMARK(BM_TwoThread_Empty_PingPong<1 << 12, 1 << 12>)->Threads(2);
// Run the benchmark
BENCHMARK_MAIN();