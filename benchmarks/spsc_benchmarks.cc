// Shared benchmark for different implementations of a SPSC queue.
#include "boost_lock_buffer/receiver.hh"
#include "boost_lock_buffer/sender.hh"
#include "fixed_size/receiver/receiver.hh"
#include "fixed_size/sender/sender.hh"
#include "utils.hh"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <benchmark/benchmark.h>
#include <cassert>
#include <iostream>
#include <thread>
#include <atomic>
#include <optional>
#include <chrono>

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

// Randomly generated name for the queue, unique to each benchmark
std::string shm_name;

static void SetupBench(const benchmark::State &state)
{
    // Randomly generate a name for the queue once
    auto now = std::chrono::high_resolution_clock::now();
    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    // Cast the result to unsigned int (will overflow but the seed will still be unique between test iterations)
    unsigned int nanoseconds = static_cast<unsigned int>(now_ns);
    srand(nanoseconds);
    shm_name = "test" + std::to_string(rand());
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
    auto sender{Tx(name, queue_size)};
    auto receiver{Rx(name, queue_size)};
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
        std::optional<Message<message_size>> received = receiver.recv();
        for (size_t i = 0; i < message_size; i++)
        {
            ASSERT(received.value().data[i] == msg.data[i]);
        }
    }
}

// Indicates sender is done setting up. Sender must be initialized before the receiver in some implementations
std::atomic<bool> two_thread_setup_done = false;

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

    spdlog::info("Running ping-pong benchmark with message size: {}, queue size: {}, and test name {}", message_size, queue_size, shm_name);

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
    // In the below, using PauseTiming() and ResumeTiming() to isolate the `send()` and `recv()` was considered,
    // but their overhead dominates short operations.
    // See: https://github.com/google/benchmark/issues/797
    if (state.thread_index() == SENDER_THREAD_ID)
    {
        auto sender{Tx(shm_name, queue_size)};
        two_thread_setup_done = true;

        for (auto _ : state)
        {
            // Retry sends if the queue is full. In this test this should not happen
            // since after each send the receiver consumes messages
            while (!sender.send(msg))
            {
            }
            while (sender.size() > 0)
            {
                spdlog::debug("Sender size: {}", sender.size());
                // If the receiver is slow, yield to let receiver run
                // to keep the queue empty
                std::this_thread::yield();
            }
        }
    }
    else if (state.thread_index() == RECEIVER_THREAD_ID)
    {
        // Wait for sender to be initialized before receiver is initialized
        while (!two_thread_setup_done)
        {
        }
        auto receiver{Rx(shm_name, queue_size)};
        for (auto _ : state)
        {
            // Poll until the receiver has received a message
            while (receiver.size() == 0)
            {
                spdlog::debug("Receiver size: {}", receiver.size());
                std::this_thread::yield();
            }
            spdlog::debug("Receiver running recv()");
            // Only time the receive operation
            auto received = receiver.recv();
            // Confirm the receiver received correct messages
            for (size_t i = 0; i < message_size; i++)
            {
                ASSERT(received.value().data[i] != msg.data[i]);
            }
        }
        // After the receiver is done, there should be no more messages in the queue
        ASSERT(receiver.size() == 0);
        spdlog::debug("Receiver done");
    }
}

// Single Threaded
// Boost Lock Buffer
#define BOOST_SINGLETHREAD_BENCH(queue_size, message_size)            \
    BENCHMARK(BM_SingleThread_Empty_PingPong<                         \
                  boost_lock_buffer::Sender<Message<message_size>>,   \
                  boost_lock_buffer::Receiver<Message<message_size>>, \
                  queue_size,                                         \
                  message_size>)                                      \
        ->Setup(SetupBench);

BOOST_SINGLETHREAD_BENCH(1 << 12, 1 << 2)
BOOST_SINGLETHREAD_BENCH(1 << 12, 1 << 6)
BOOST_SINGLETHREAD_BENCH(1 << 12, 1 << 8)
BOOST_SINGLETHREAD_BENCH(1 << 12, 1 << 12)
BOOST_SINGLETHREAD_BENCH(1 << 20, 1 << 2)
BOOST_SINGLETHREAD_BENCH(1 << 20, 1 << 6)
BOOST_SINGLETHREAD_BENCH(1 << 20, 1 << 8)
BOOST_SINGLETHREAD_BENCH(1 << 20, 1 << 12)

// Koi
#define KOI_SINGLETHREAD_BENCH(queue_size, message_size)   \
    BENCHMARK(BM_SingleThread_Empty_PingPong<              \
                  koi::KoiSender<Message<message_size>>,   \
                  koi::KoiReceiver<Message<message_size>>, \
                  queue_size,                              \
                  message_size>)                           \
        ->Setup(SetupBench);

KOI_SINGLETHREAD_BENCH(1 << 12, 1 << 2)
KOI_SINGLETHREAD_BENCH(1 << 12, 1 << 6)
KOI_SINGLETHREAD_BENCH(1 << 12, 1 << 8)
KOI_SINGLETHREAD_BENCH(1 << 12, 1 << 12)
KOI_SINGLETHREAD_BENCH(1 << 20, 1 << 2)
KOI_SINGLETHREAD_BENCH(1 << 20, 1 << 6)
KOI_SINGLETHREAD_BENCH(1 << 20, 1 << 8)
KOI_SINGLETHREAD_BENCH(1 << 20, 1 << 12)

// Multi Threaded
// Boost Lock Buffer
#define BOOST_MULTITHREAD_BENCH(queue_size, message_size)             \
    BENCHMARK(BM_TwoThread_Empty_PingPong<                            \
                  boost_lock_buffer::Sender<Message<message_size>>,   \
                  boost_lock_buffer::Receiver<Message<message_size>>, \
                  queue_size,                                         \
                  message_size>)                                      \
        ->Threads(2)                                                  \
        ->Setup(SetupBench)                                           \
        ->Teardown(TeardownTwoThread);

BOOST_MULTITHREAD_BENCH(1 << 12, 1 << 2)
BOOST_MULTITHREAD_BENCH(1 << 12, 1 << 6)
BOOST_MULTITHREAD_BENCH(1 << 12, 1 << 8)
BOOST_MULTITHREAD_BENCH(1 << 12, 1 << 12)
BOOST_MULTITHREAD_BENCH(1 << 20, 1 << 2)
BOOST_MULTITHREAD_BENCH(1 << 20, 1 << 6)
BOOST_MULTITHREAD_BENCH(1 << 20, 1 << 8)
BOOST_MULTITHREAD_BENCH(1 << 20, 1 << 12)

// Koi
#define KOI_MULTITHREAD_BENCH(queue_size, message_size)    \
    BENCHMARK(BM_TwoThread_Empty_PingPong<                 \
                  koi::KoiSender<Message<message_size>>,   \
                  koi::KoiReceiver<Message<message_size>>, \
                  queue_size,                              \
                  message_size>)                           \
        ->Threads(2)                                       \
        ->Setup(SetupBench)                                \
        ->Teardown(TeardownTwoThread);

KOI_MULTITHREAD_BENCH(1 << 12, 1 << 2)
KOI_MULTITHREAD_BENCH(1 << 12, 1 << 6)
KOI_MULTITHREAD_BENCH(1 << 12, 1 << 8)
KOI_MULTITHREAD_BENCH(1 << 12, 1 << 12)
KOI_MULTITHREAD_BENCH(1 << 20, 1 << 2)
KOI_MULTITHREAD_BENCH(1 << 20, 1 << 6)
KOI_MULTITHREAD_BENCH(1 << 20, 1 << 8)
KOI_MULTITHREAD_BENCH(1 << 20, 1 << 12)

// Run the benchmarks
BENCHMARK_MAIN();