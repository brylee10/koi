#include <benchmark/benchmark.h>
#include <thread>

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
        // Effectively creates a read/write barrier to prevent the compiler from optimizing out the memcpy
        // See: https://github.com/google/benchmark/blob/main/docs/user_guide.md#preventing-optimization
        // On barriers: https://developer.arm.com/documentation/den0013/d/Memory-Ordering/Memory-barriers/Linux-use-of-barriers
        benchmark::DoNotOptimize(std::memcpy(const_cast<char *>(dst), const_cast<const char *>(src), data_size_bytes));
        benchmark::ClobberMemory();
    }
}

// Memcopy sets a lower bound for a given data size
BENCHMARK(BM_memcpy<1 << 4>);
BENCHMARK(BM_memcpy<1 << 6>);
BENCHMARK(BM_memcpy<1 << 8>);
BENCHMARK(BM_memcpy<1 << 12>);

// Run the benchmark
BENCHMARK_MAIN();