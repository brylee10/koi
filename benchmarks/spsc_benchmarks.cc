// Shared benchmark for different implementations of a SPSC queue.
#include "boost_lock_buffer/receiver.hh"
#include "boost_lock_buffer/sender.hh"
#include "utils.hh"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <benchmark/benchmark.h>
#include <cassert>
#include <iostream>
#include <thread>
#include <atomic>
#include <optional>

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
template <typename Tx, typename Rx, size_t queue_size, size_t message_size>
void BM_SingleThread_Empty_PingPong(benchmark::State &state)
{
    spdlog::set_level(spdlog::level::err);

    // Randomly generate a name for the queue
    const std::string name = "test" + std::to_string(rand());
    auto sender{Tx(queue_size, name)};
    auto receiver{Rx(name)};
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

// Randomly generated name for the queue, unique to each benchmark
std::string two_thread_name;
// Indicates sender is done setting up. Sender must be initialized before the receiver in some implementations
std::atomic<bool> two_thread_setup_done = false;

static void SetupTwoThread(const benchmark::State &state)
{
    // Randomly generate a name for the queue once
    two_thread_name = "test" + std::to_string(rand());
}

static void TeardownTwoThread(const benchmark::State &state)
{
    // Reset the setup done flag
    two_thread_setup_done = false;
}

template <typename Tx, typename Rx, size_t queue_size, size_t message_size>
void BM_TwoThread_Empty_PingPong(benchmark::State &state)
{
    spdlog::set_level(spdlog::level::err);

    constexpr int SENDER_THREAD_ID = 0;
    constexpr int RECEIVER_THREAD_ID = 1;

    spdlog::info("Running ping-pong benchmark with message size: {}, queue size: {}, and test name {}", message_size, queue_size, two_thread_name);

    if (state.threads() > 2)
    {
        spdlog::error("This benchmark only supports 2 threads");
        return;
    }

    Message<message_size> msg = {};
    // Assign 1 to all values in the message
    for (size_t i = 0; i < message_size; i++)
    {
        msg.data[i] = 1;
    }
    if (state.thread_index() == SENDER_THREAD_ID)
    {
        auto sender{Tx(two_thread_name, queue_size)};
        two_thread_setup_done = true;

        for (auto _ : state)
        {
            // Retry sends if the queue is full
            while (!sender.send(msg))
            {
            }

            // Do not send until the receiver has read the message
            while (sender.size() > 0)
            {
            }
        }
    }
    else if (state.thread_index() == RECEIVER_THREAD_ID)
    {
        // Wait for sender to be initialized before receiver is initialized
        while (!two_thread_setup_done)
        {
        }
        auto receiver{Rx(two_thread_name)};
        for (auto _ : state)
        {
            // Poll until the sender has sent a message
            std::optional<Message<message_size>> received = std::nullopt;
            do
            {
                received = receiver.recv();
            } while (!received.has_value());
            // Confirm the receiver received correct messages
            for (size_t i = 0; i < message_size; i++)
            {
                // Disabled in release mode
                if (received.value().data[i] != msg.data[i])
                {
                    std::cerr << "Mismatch at index: " << i << std::endl;
                    std::cerr << "Expected: " << msg << std::endl;
                    std::cerr << "Received: " << received.value() << std::endl;
                    throw std::runtime_error("Mismatch in received message");
                }
            }
        }
        // After the receiver is done, there should be no more messages in the queue
        ASSERT(receiver.size() == 0);
    }
}

// Single Threaded
#define SINGLETHREAD_BENCH(queue_size, message_size)              \
    BENCHMARK(BM_SingleThread_Empty_PingPong<                     \
              boost_lock_buffer::Sender<Message<message_size>>,   \
              boost_lock_buffer::Receiver<Message<message_size>>, \
              queue_size,                                         \
              message_size>);

// // Register all singlethreaded benchmarks
// SINGLETHREAD_BENCH(1 << 20, 1 << 2)
// SINGLETHREAD_BENCH(1 << 20, 1 << 6)
// SINGLETHREAD_BENCH(1 << 20, 1 << 8)
// SINGLETHREAD_BENCH(1 << 20, 1 << 12)
// SINGLETHREAD_BENCH(1 << 12, 1 << 2)
// SINGLETHREAD_BENCH(1 << 12, 1 << 6)
// SINGLETHREAD_BENCH(1 << 12, 1 << 8)
// SINGLETHREAD_BENCH(1 << 12, 1 << 12)

// Multi Threaded
// Boost Lock Buffer
#define MULTITHREAD_BENCH(queue_size, message_size)                   \
    BENCHMARK(BM_TwoThread_Empty_PingPong<                            \
                  boost_lock_buffer::Sender<Message<message_size>>,   \
                  boost_lock_buffer::Receiver<Message<message_size>>, \
                  queue_size,                                         \
                  message_size>)                                      \
        ->Threads(2)                                                  \
        ->Setup(SetupTwoThread)                                       \
        ->Teardown(TeardownTwoThread);

// Register all multithreaded benchmarks
// MULTITHREAD_BENCH(1 << 20, 1 << 2)
// MULTITHREAD_BENCH(1 << 20, 1 << 6)
// MULTITHREAD_BENCH(1 << 20, 1 << 8)
// MULTITHREAD_BENCH(1 << 20, 1 << 12)
// MULTITHREAD_BENCH(1 << 12, 1 << 6)
// MULTITHREAD_BENCH(1 << 12, 1 << 6)
// MULTITHREAD_BENCH(1 << 12, 1 << 8)
// MULTITHREAD_BENCH(1 << 12, 1 << 12)
MULTITHREAD_BENCH(1 << 12, 1 << 2)
MULTITHREAD_BENCH(1 << 12, 1 << 2)
MULTITHREAD_BENCH(1 << 12, 1 << 2)
MULTITHREAD_BENCH(1 << 12, 1 << 2)
MULTITHREAD_BENCH(1 << 12, 1 << 2)

// Run the benchmarks
BENCHMARK_MAIN();