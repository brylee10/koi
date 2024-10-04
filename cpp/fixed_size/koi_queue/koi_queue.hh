#pragma once

#include "koi_utils.hh"

#include <string>
#include <optional>
#include <atomic>

// `MessageHeader` precedes each message block
struct MessageHeader
{
    // Indicates if a block has been written to
    std::atomic<bool> occupied;
};

// Maximum size of message message block selected as a multiple of `CACHE_LINE_BYTES`,
// arbitrarily set to 2^10 * `CACHE_LINE_BYTES`
constexpr size_t MAX_MESSAGE_BLOCK_BYTES = (1 << 10) * CACHE_LINE_BYTES;
constexpr size_t MAX_MESSAGE_SIZE_BYTES = MAX_MESSAGE_BLOCK_BYTES - sizeof(MessageHeader);

enum KoiQueueRet
{
    // Assigned 0 as in "False" for unsuccessful operation
    QUEUE_FULL = 0,
    OK = 1,
};

// Contains read/write metadata on one cacheline
struct ControlBlockInner
{
    // Either read or write offset
    std::atomic<size_t> offset;
    // Duplicate the read only fields for the read/write cache line for prefetching.
    // The values are identical between the read/write cache lines.
    size_t user_shm_size;
    size_t message_block_sz;
};

// Shared information among all processes encoded in the shared memory
struct ControlBlock
{
    // The below are aligned to the nearest cache line to avoid false sharing
    // `size_t` used for offsets to avoid negative values
    // "tail"
    alignas(CACHE_LINE_BYTES) ControlBlockInner write;
    // "head"
    alignas(CACHE_LINE_BYTES) ControlBlockInner read;
};

inline std::ostream &operator<<(std::ostream &os, const ControlBlockInner &b)
{
    os << "Offset: " << b.offset << ", user shm size: " << b.user_shm_size
       << ", message block sz: " << b.message_block_sz << std::endl;
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const ControlBlock &b)
{
    os << "Writer: " << b.write << ", Reader: " << b.read << std::endl;
    return os;
}

// Forward declaration of KoiQueueRAII
template <typename T>
class KoiQueueRAII;

// Terminology: "message block" = "message header" + "message"
template <typename T>
class KoiQueue
{
public:
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
    // Returns the number of messages in the queue
    size_t size() const;
    // Returns max number of messages the queue can hold
    size_t capacity() const;

protected:
    // `buffer_bytes` will be rounded up to the nearest multiple of `CACHE_LINE_BYTES`
    explicit KoiQueue(const std::string name, size_t buffer_bytes);
    virtual ~KoiQueue();

    // Returns `KoiQueueRet::QUEUE_FULL` if the queue is full, otherwise `KoiQueueRet::OK`
    KoiQueueRet send(T message);
    std::optional<T> recv();

    // Exception safety. Marked as `noexcept` such that an exception is not thrown during stack unwinding which leads to terminate.
    void cleanup_shm() noexcept;

private:
    // The size of a "message block" (the message header + the message itself)
    // Round message block size up to a multiple of cache line
    static constexpr size_t message_block_sz_ = size_rounded_up_to_pow_2_cache_line(sizeof(MessageHeader) + sizeof(T));
    static constexpr size_t message_sz = sizeof(T);
    ControlBlock *control_block_;

    struct ShmMetadata
    {
        std::string shm_name;
        int shm_fd;
        char *shm_ptr;
        // `shm_ptr` + sizeof(ControlBlock) (aligned to the nearest cache line)
        // Start of implicit ring buffer data structure
        char *user_shm_start;
        size_t message_sz = sizeof(T);
        size_t total_shm_size;
        size_t user_shm_size;
    };

    ShmMetadata shm_metadata_;

    // Initialization order is `open_shm` then `init_shm`
    int open_shm();
    int init_shm(int);

    // Allow `KoiQueueRAII` to access private and protectedmembers, particularly `cleanup_shm`
    friend class KoiQueueRAII<T>;
};

// RAII class to optionally cleanup the shared memory segment
// Typically this is not desirable so the shm segment can be used by other processes later
// but this is useful for test cleanup
template <typename T>
class KoiQueueRAII : public KoiQueue<T>
{
public:
    // `explicit` constructor optional since the class takes two arguments which is difficult
    // to accidentally invoke with an implicit conversion
    explicit KoiQueueRAII(const std::string name, size_t buffer_bytes) : KoiQueue<T>(name, buffer_bytes)
    {
    }

    ~KoiQueueRAII()
    {
        // `cleanup_shm` is not called in the default `KoiQueue` destructor
        cleanup_shm();
    }

    using KoiQueue<T>::send;
    using KoiQueue<T>::recv;

private:
    using KoiQueue<T>::cleanup_shm;
};

// Ensure all dependencies are declared
#include "koi_queue.tcc"