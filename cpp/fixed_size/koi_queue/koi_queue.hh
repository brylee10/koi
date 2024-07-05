#pragma once

#include <string>

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

struct Message
{
};

constexpr size_t CACHE_LINE_BYTES = 64;
constexpr size_t MAX_MESSAGE_SIZE_BYTES = CACHE_LINE_BYTES - sizeof(MessageHeader);

template <typename T>
class KoiQueue
{
public:
    // `buffer_bytes` will be rounded up to the nearest multiple of `CACHE_LINE_BYTES`
    KoiQueue(const std::string_view name, size_t buffer_bytes);
    ~KoiQueue();

    void init_shm();
    void send(T message);
    T recv();
    size_t shm_size() const;

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
};

#include "koi_queue.tcc"