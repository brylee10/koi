
#include "koi_queue.hh"
#include "logger.hh"

#include "spdlog/spdlog.h"

#include <iostream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <unistd.h>

template <typename T>
KoiQueue<T>::KoiQueue(const std::string_view shm_name, size_t shm_size)
{
    // `send`/`recv` will do a bitwise memcpy of T into the shared memory
    constexpr size_t message_sz = sizeof(T);
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
    static_assert(message_sz <= MAX_MESSAGE_SIZE_BYTES, "Message size is larger than the max message size");

    load_spdlog_level();
    shm_metadata_.shm_name = std::string(shm_name);
    // `buffer_bytes` must be at least `sizeof(ControlBlock)`
    // The first `sizeof(ControlBlock)` bytes are reserved for the control block
    if (shm_size < sizeof(ControlBlock))
    {
        throw std::invalid_argument("buffer_bytes must be at least sizeof(ControlBlock), " +
                                    std::to_string(sizeof(ControlBlock)) + " bytes");
    }

    // The `shm_size` must be a power of 2
    if ((shm_size & (shm_size - 1)) != 0)
    {
        throw std::invalid_argument("shm_size provided " + std::to_string(shm_size) +
                                    " is not a power of 2");
    }

    shm_metadata_.shm_size = shm_size;
    control_block_.write_offset = 0;
    control_block_.read_offset = 0;
    message_block_cnt_ = 0;
    // Round message block size up to the nearest cache line
    message_block_sz_ = (message_sz + (CACHE_LINE_BYTES - 1)) & ~(CACHE_LINE_BYTES - 1);
    spdlog::debug("Message Size bytes: {}, rounded up to nearest cache line message_block_sz_: {}",
                  message_sz, message_block_sz_);
}

template <typename T>
KoiQueue<T>::~KoiQueue()
{
    spdlog::debug("Starting KoiQueue destructor");
    if (shm_unlink(shm_metadata_.shm_name.c_str()) == -1)
    {
        // `shm_unlink` can fail if the shared memory was already unlinked by the client/server
    }

    // Can only unmap if the shared memory was successfully mapped
    // This should be called after `shm_unlink` because the `mmap` occurs after
    // the `shm_open` call in `init_shm`.
    if (shm_metadata_.shm_ptr != nullptr)
    {
        if (munmap(shm_metadata_.shm_ptr, shm_metadata_.shm_size) == -1)
        {
            // munmap can fail if the constructor threw an exception
            // before the shared memory was mapped
            // spdlog::error("munmap failed");
        }
    }
}

// This must be called before any other operation to initialize the shared memory.
// Not called in the constructor to avoid throwing exceptions in the constructor.
template <typename T>
void KoiQueue<T>::init_shm()
{
    spdlog::debug("Starting init_shm");
    int shm_fd = shm_open(shm_metadata_.shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        throw std::runtime_error("Failed to open shared memory");
    }

    shm_metadata_.shm_fd = shm_fd;

    if (ftruncate(shm_fd, shm_metadata_.shm_size) == -1)
    {
        spdlog::error("ftruncate failed with errno: {}", errno);
        spdlog::error("fd: {}, shm_size: {}", shm_fd, shm_metadata_.shm_size);

        // `ftruncate` on an already open file descriptor can fail with EINVAL
        // https://stackoverflow.com/questions/20320742/ftruncate-failed-at-the-second-time
        if (errno != EINVAL)
        {
            perror("ftruncate");
            throw std::runtime_error("ftruncate failed");
        }
    }

    char *shm_ptr = static_cast<char *>(mmap(NULL, shm_metadata_.shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (shm_ptr == MAP_FAILED)
    {
        perror("mmap");
        throw std::runtime_error("Failed to map shared memory");
    }

    shm_metadata_.shm_ptr = shm_ptr;
}

template <typename T>
KoiQueueRet KoiQueue<T>::send(T message)
{
    if (is_full())
    {
        return KoiQueueRet::QUEUE_FULL;
    }

    constexpr size_t message_sz = sizeof(T);
    MessageHeader header = {message_sz};

    char *start = shm_metadata_.shm_ptr + control_block_.write_offset;
    char *header_end = start + sizeof(MessageHeader);
    // Copy the header into the shared memory
    std::copy(reinterpret_cast<char *>(&header),
              reinterpret_cast<char *>(&header) + sizeof(MessageHeader), start);

    // Copy the message into the shared memory
    std::copy(reinterpret_cast<char *>(&message),
              reinterpret_cast<char *>(&message) + message_sz, header_end);

    // Every message is rounded up to the nearest cache line (`message_block_sz_`)
    control_block_.write_offset += message_block_sz_;
    // Wrap around the ring buffer
    control_block_.write_offset = control_block_.write_offset & (shm_metadata_.shm_size - 1);
    message_block_cnt_++;
    return KoiQueueRet::OK;
}

template <typename T>
std::optional<T> KoiQueue<T>::recv()
{
    if (control_block_.read_offset == control_block_.write_offset)
    {
        return std::nullopt;
    }

    char *start = shm_metadata_.shm_ptr + control_block_.read_offset;
    MessageHeader *header = reinterpret_cast<MessageHeader *>(start);
    size_t message_sz = header->message_sz;

    char *header_end = start + sizeof(MessageHeader);
    T message;
    std::copy(header_end, header_end + message_sz, reinterpret_cast<char *>(&message));

    // Every message is rounded up to the nearest cache line (`message_block_sz_`)
    control_block_.read_offset += message_block_sz_;
    // Wrap around the ring buffer
    control_block_.read_offset = control_block_.read_offset & (shm_metadata_.shm_size - 1);
    message_block_cnt_--;
    return message;
}

template <typename T>
size_t KoiQueue<T>::shm_size() const
{
    return shm_metadata_.shm_size;
}

template <typename T>
size_t KoiQueue<T>::curr_queue_sz_bytes() const
{

    size_t curr_queue_sz = (control_block_.write_offset -
                            control_block_.read_offset + shm_metadata_.shm_size) &
                           (shm_metadata_.shm_size - 1);
    if (curr_queue_sz == 0 && message_block_cnt_ != 0)
    {
        // Special case where the queue is full and the read and write offsets are the same
        return shm_metadata_.shm_size;
    }
    return curr_queue_sz;
}

template <typename T>
size_t KoiQueue<T>::shm_remaining_bytes() const
{
    spdlog::debug("shm_size: {}, curr_queue_sz_bytes: {}", shm_metadata_.shm_size, curr_queue_sz_bytes());
    return shm_metadata_.shm_size - curr_queue_sz_bytes();
}

template <typename T>
size_t KoiQueue<T>::message_block_sz_bytes() const
{
    return message_block_sz_;
}

template <typename T>
bool KoiQueue<T>::is_full() const
{
    // The check for message size not larger than the max message size is done statically in the
    // constructor
    size_t curr_queue_sz = curr_queue_sz_bytes();
    spdlog::debug("curr_queue_sz: {}, message_block_cnt_: {}, shm_size: {}",
                  curr_queue_sz, message_block_cnt_, shm_metadata_.shm_size);
    return curr_queue_sz >= shm_metadata_.shm_size && message_block_cnt_ != 0;
}