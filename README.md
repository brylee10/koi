# Koi
A single producer single consumer (SPSC) high performance message queue.

**Properties**:
* Supports variable length messages, similar to UNIX message queues based on linked lists
* Does not have a handshake joining procedure, taking inspiration from UNIX named pipes and other file based IPC mechanisms. This allows easy queue joining and queue liveness even when participants disconnect
* Backed by a persistent file, allowing queue sessions to resume even when all participants have disconnected
* Performs in-line with other top message queues in benchmarks for message sizes up to 64KB.

