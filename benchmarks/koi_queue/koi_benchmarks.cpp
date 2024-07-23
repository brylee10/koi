#include "receiver.hh"
#include "sender.hh"

#include <benchmark/benchmark.h>
#include <thread>

template <size_t queue_size, size_t message_size>
void BM_PingPong(benchmark::State &state)
{
    struct Message
    {
        char data[message_size];
    };
    KoiSender sender = KoiSender<Message>("test", queue_size);
    KoiReceiver receiver = KoiReceiver<Message>("test", queue_size);
    Message msg = {};
    for (auto _ : state)
    {
        sender.send(msg);
        receiver.recv();
    }
    // Cleanup
    sender.cleanup_shm();
}

// Register the function as a benchmark
BENCHMARK(BM_PingPong<1 << 12, 64>);
// Register the function as a benchmark
BENCHMARK(BM_PingPong<1 << 12, 128>);
// Register the function as a benchmark
BENCHMARK(BM_PingPong<1 << 12, 256>);
// Run the benchmark
BENCHMARK_MAIN();