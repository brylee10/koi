# Koi
A single producer single consumer (SPSC) high performance message queue.

**Properties**:
* Supports variable length messages, similar to UNIX message queues based on linked lists
* Does not have a handshake joining procedure, taking inspiration from UNIX named pipes and other file based IPC mechanisms. This allows easy queue joining and queue liveness even when participants disconnect
* Backed by a persistent file, allowing queue sessions to resume even when all participants have disconnected
* Performs in-line with other top message queues in benchmarks for message sizes up to 64KB.

# Running
Clone the repository locally and then run

```shell
# Koi build uses a git submodule pointing to `boost-cmake`, pull this repository and any nested submodules 
git submodules update --init --recursive
cmake .
make
```

The unit tests located in `tests/` can be run via:
```shell
# Runs Koi fixed size queue unit tests
bin/test/koi_fixed_size
```

The benchmarks located in `benchmarks` can be run via:
```shell
# Runs baseline memcpy benchmark
bin/benchmarks/memcpy
# Runs Koi fixed size benchmark
bin/benchmarks/koi_fixed_size
```

# Benchmarks
Koi is benchmarked against other C++ SPSC implementations, namely Boost SPSC. Others can be added in the future. Benchmarks test throughput (messages / sec) with different message sizes across a number of iterations using [Google's C++ microbenchmark framework](https://github.com/google/benchmark/tree/main).

Benchmarks are run on a MacOS machine with a M1 Max chip with 10 cores and the following specifications, as reported by Google Benchmarks:
```
Run on (10 X 24 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB
  L1 Instruction 128 KiB
  L2 Unified 4096 KiB (x10)
```

This benchmarks the following:
- A lock-based Bounded Buffer, implemented with Boost primitives (`benchmarks/boost_lock_buffer`): This is a baseline, as generally lock based algorithmns which internally issue futex system calls will generally perform worse than lock-free algorithms with atomics alone.

# Repository Structure
- Unit tests: Located under `tests/fixed_size/koi_queue` for Koi fixed size queue unit tests. These test basic single threaded ping pongs as well as multi process ping pongs.
- Benchmarks: Benchmarks are run via Google Benchmarks and located under the `benchmarks` folder. Benchmarks generally measure the time for one ping-pong for varying queue sizes and message sizes (in bytes).  

# Implementations
The below describes the implementations of thevarious benchmarks at a high level. More context is given on the Koi queue implementation in the [Theory](#theory) section.

All implementations require the following guarantees:
- Senders receive an indicator value when attempting to send while the buffer is full
- Receivers receive an indicator value when attempting to read while the buffer is empty
- Receivers receive messages in the order in which they are sent and all messages are received
- The queues should only be used by one sender and one producer (SPSC)
- A message can only be read by one receiver

## Boost Bounded Buffer (Locks)
The boost bounded buffer (circular buffer) is based on the implementation [provided by Boost here](https://www.boost.org/doc/libs/1_85_0/doc/html/circular_buffer/examples.html). This particular implementation uses locks, so it is a baseline benchmark which is not considered performance optimized. The Boost managed shared memory library allows dynamic allocation of named objects inside a named shared memory segment, in this case the `boost::circular_buffer`. 

# Theory