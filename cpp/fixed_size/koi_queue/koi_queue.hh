#pragma once

#include <string>
#include <optional>

enum KoiQueueRet
{
    OK = 0,
    QUEUE_FULL = 1,
};

// This container holds no type information and only passes raw bytes
struct ControlBlock
{
    // `size_t` used for offsets to avoid negative values
    // "tail"
    size_t write_offset;
    // "head"
    size_t read_offset;
};

// `MessageHeader` precedes each message block
struct MessageHeader
{
    size_t message_sz;
};

constexpr size_t CACHE_LINE_BYTES = 64;
// Maximum size of message message block selected as a multiple of `CACHE_LINE_BYTES`,
// arbitrarily set to 2^10 * `CACHE_LINE_BYTES`
constexpr size_t MAX_MESSAGE_BLOCK_BYTES = (1 << 10) * CACHE_LINE_BYTES;
constexpr size_t MAX_MESSAGE_SIZE_BYTES = MAX_MESSAGE_BLOCK_BYTES - sizeof(MessageHeader);

// Terminology: "message block" = "message header" + "message"
template <typename T>
class KoiQueue
{
public:
    // `buffer_bytes` will be rounded up to the nearest multiple of `CACHE_LINE_BYTES`
    KoiQueue(const std::string_view name, size_t buffer_bytes);
    ~KoiQueue();

    void init_shm();
    // Returns `KoiQueueRet::QUEUE_FULL` if the queue is full, otherwise `KoiQueueRet::OK`
    KoiQueueRet send(T message);
    std::optional<T> recv();
    // Returns total size of the shm segment
    size_t shm_size() const;
    // Returns total bytes remaining in the shm segment
    size_t shm_remaining_bytes() const;
    // Returns the message block size in bytes
    size_t message_block_sz_bytes() const;
    // Returns the current queue size in bytes
    size_t curr_queue_sz_bytes() const;
    // Returns if the queue is full
    bool is_full() const;

private:
    ControlBlock control_block_;

    struct ShmMetadata
    {
        // shm size is allocated shared memory size
        size_t shm_size;
        std::string shm_name;
        int shm_fd;
        char *shm_ptr;
    };

    ShmMetadata shm_metadata_;
    // The size of a "message block" (the message header + the message itself)
    size_t message_block_sz_;
    // Current number of message blocks in the queue
    size_t message_block_cnt_;
};

#include "koi_queue.tcc"