# Koi
A single producer single consumer (SPSC) high performance message queue.

**Properties**:
* Supports variable length messages, similar to UNIX message queues based on linked lists
* Does not have a handshake joining procedure, taking inspiration from UNIX named pipes and other file based IPC mechanisms. This allows easy queue joining and queue liveness even when participants disconnect
* Backed by a persistent file, allowing queue sessions to resume even when all participants have disconnected
* Performs in-line with other top message queues in benchmarks for message sizes up to 64KB.

# Running
Clone the repository locally and then run

```
cmake .
make
```

# Benchmarks
Koi is benchmarked against other C++ SPSC implementations, namely Boost SPSC. Others can be added in the future.

Benchmarks test throughput (messages / sec) with different message sizes across a number of iterations.

# Theory

