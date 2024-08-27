#include "receiver.hh"
#include "sender.hh"
#include "utils.hh"

#include <benchmark/benchmark.h>
#include <thread>

using namespace koi;

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
        std::optional<Message> val = receiver.recv();
        ASSERT(val.has_value());
        Message val_inner = val.value();
        for (size_t i = 0; i < message_size; i++)
        {
            ASSERT(val_inner.data[i] == msg.data[i]);
        }
    }
    // Cleanup
    sender.cleanup_shm();
}

// Koi benchmarks (parametersized by queue size and message size)
BENCHMARK(BM_PingPong<1 << 20, 1 << 2>);
BENCHMARK(BM_PingPong<1 << 20, 1 << 6>);
BENCHMARK(BM_PingPong<1 << 20, 1 << 8>);
BENCHMARK(BM_PingPong<1 << 20, 1 << 10>);
BENCHMARK(BM_PingPong<1 << 20, 1 << 12>);
BENCHMARK(BM_PingPong<1 << 12, 1 << 0>);
BENCHMARK(BM_PingPong<1 << 12, 1 << 2>);
BENCHMARK(BM_PingPong<1 << 12, 1 << 6>);
BENCHMARK(BM_PingPong<1 << 12, 1 << 8>);
BENCHMARK(BM_PingPong<1 << 12, 1 << 10>);
BENCHMARK(BM_PingPong<1 << 12, 1 << 12>);
BENCHMARK(BM_PingPong<1 << 2, 1 << 0>);

// Run the benchmark
BENCHMARK_MAIN();