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
        std::optional<Message> val = receiver.recv();
        assert(val.has_value());
        Message val_inner = val.value();
        for (size_t i = 0; i < message_size; i++)
        {
            assert(val_inner.data[i] == msg.data[i]);
        }
    }
    // Cleanup
    sender.cleanup_shm();
}

template <size_t data_size_bytes>
void BM_memcpy(benchmark::State &state)
{
    // Memcpy `data_size_bytes` of random data
    char *src = new char[data_size_bytes];
    char *dst = new char[data_size_bytes];

    // Set data to random values
    for (size_t i = 0; i < data_size_bytes; i++)
    {
        src[i] = rand() % 256;
    }
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(std::memcpy(const_cast<char *>(dst), const_cast<const char *>(src), data_size_bytes));
        benchmark::ClobberMemory();
    }
}

// Register the function as a benchmark
// A lower bound
BENCHMARK(BM_memcpy<1 << 4>);
BENCHMARK(BM_memcpy<1 << 6>);
BENCHMARK(BM_memcpy<1 << 8>);
BENCHMARK(BM_memcpy<1 << 12>);
// Koi benchmarks
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