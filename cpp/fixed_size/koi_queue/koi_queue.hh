#pragma once

#include <string>
#include <optional>
#include <atomic>

enum KoiQueueRet
{
    OK = 0,
    QUEUE_FULL = 1,
};

// Shared information among all processes encoded in the shared memory
struct ControlBlock
{
    // `user_shm_size` is user allocated shared memory size
    // (not including additional Koi queue metadata, like the control block size)
    // The real size of the shm is CACHE_LINE_SIZE + `user_shm_size`, where the first
    // cache line holds the `ControlBlock`
    size_t user_shm_size;
    // The size of a "message block" (the message header + the message itself)
    size_t message_block_sz;
    // `size_t` used for offsets to avoid negative values
    // "tail"
    std::atomic<size_t> write_offset;
    // "head"
    std::atomic<size_t> read_offset;
    // Current number of message blocks in the queue
    std::atomic<size_t> message_block_cnt;
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

// Forward declaration of KoiQueueRAII
template <typename T>
class KoiQueueRAII;

// Terminology: "message block" = "message header" + "message"
template <typename T>
class KoiQueue
{
public:
    // `buffer_bytes` will be rounded up to the nearest multiple of `CACHE_LINE_BYTES`
    KoiQueue(const std::string_view name, size_t buffer_bytes);
    ~KoiQueue();

    // Returns `KoiQueueRet::QUEUE_FULL` if the queue is full, otherwise `KoiQueueRet::OK`
    KoiQueueRet send(T message);
    std::optional<T> recv();
    // Returns total size of the shm segment that the user allocated
    // This is caveated by the fact the actual size of the shm segment includes the control block,
    // but this is opaque to the user
    size_t user_shm_size() const;
    // Returns total bytes remaining in the shm segment that the user allocated
    size_t shm_remaining_bytes() const;
    // Returns the message block size in bytes
    constexpr size_t message_block_sz_bytes() const;
    // Returns the current queue size in bytes
    size_t curr_queue_sz_bytes() const;
    // Returns if the queue is full
    bool is_full() const;
    // Returns if the queue is empty
    bool is_empty() const;

private:
    // The size of a "message block" (the message header + the message itself)
    // Round message block size up to the nearest cache line
    static constexpr size_t message_block_sz_ = (sizeof(T) + (CACHE_LINE_BYTES - 1)) & ~(CACHE_LINE_BYTES - 1);
    ControlBlock *control_block_;

    struct ShmMetadata
    {
        std::string shm_name;
        int shm_fd;
        char *shm_ptr;
        // `shm_ptr` + sizeof(ControlBlock) (aligned to the nearest cache line)
        char *user_shm_start;
        size_t total_shm_size;
        size_t user_shm_size;
    };

    ShmMetadata shm_metadata_;

    // Initialization order is `open_shm` then `init_shm`
    int open_shm();
    int init_shm();
    void cleanup_shm();

    // Allow `KoiQueueRAII` to access private members, particularly `cleanup_shm`
    friend class KoiQueueRAII<T>;
};

// RAII class to optionally cleanup the shared memory segment
// Typically this is not desirable so the shm segment can be used by other processes later
// but this is useful for test cleanup
template <typename T>
class KoiQueueRAII
{
public:
    // `explicit` constructor optional since the class takes two arguments which is difficult
    // to accidentally invoke with an implicit conversion
    explicit KoiQueueRAII(const std::string_view name, size_t buffer_bytes) : queue_(name, buffer_bytes)
    {
    }

    ~KoiQueueRAII()
    {
        queue_.~KoiQueue();
        // `cleanup_shm` is not called in the default `KoiQueue` destructor
        queue_.cleanup_shm();
    }

    // Dereference operator
    KoiQueue<T> &operator*()
    {
        return queue_;
    }

    const KoiQueue<T> &operator*() const
    {
        return queue_;
    }

    // Arrow operator
    KoiQueue<T> *operator->()
    {
        return &queue_;
    }

    const KoiQueue<T> *operator->() const
    {
        return &queue_;
    }

private:
    KoiQueue<T> queue_;
};

#include "koi_queue.tcc"