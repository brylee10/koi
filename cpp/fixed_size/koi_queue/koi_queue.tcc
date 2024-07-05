
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
KoiQueue<T>::KoiQueue(const std::string_view shm_name, size_t buffer_bytes)
{
    load_spdlog_level();
    shm_metadata_.shm_name = std::string(shm_name);
    // `buffer_bytes` must be at least `sizeof(ControlBlock)`
    // The first `sizeof(ControlBlock)` bytes are reserved for the control block
    if (buffer_bytes < sizeof(ControlBlock))
    {
        throw std::invalid_argument("buffer_bytes must be at least sizeof(ControlBlock), " +
                                    std::to_string(sizeof(ControlBlock)) + " bytes");
    }

    // Round up `buffer_bytes` to the nearest `CACHE_LINE_BYTES`
    size_t buffer_bytes_rounded = (buffer_bytes + CACHE_LINE_BYTES - 1) & ~(CACHE_LINE_BYTES - 1);

    spdlog::debug("buffer_bytes: {} after rounding to a multiple of `CACHE_LINE_BYTES` is {}",
                  buffer_bytes, buffer_bytes_rounded);

    shm_metadata_.shm_size = buffer_bytes_rounded;
    control_block_.write_offset = 0;
    control_block_.read_offset = 0;
}

template <typename T>
KoiQueue<T>::~KoiQueue()
{
    spdlog::debug("Starting KoiQueue destructor");
    if (shm_unlink(shm_metadata_.shm_name.c_str()) == -1)
    {
        // `shm_unlink` can fail if the shared memory was already unlinked by the client/server
        // std::cerr << "shm_unlink failed" << std::endl;
    }

    // Can only unmap if the shared memory was successfully mapped
    // This should be called after `shm_unlink` because the `mmap` occurs after
    // the `shm_open` call in `init_shm`.
    if (shm_metadata_.shm_ptr != nullptr)
    {
        if (munmap(shm_metadata_.shm_ptr, shm_metadata_.shm_size) == -1)
        {
            // std::cerr << "munmap failed" << std::endl;
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
void KoiQueue<T>::send(T message)
{
    // Ensure the message size is not larger than the max message size
    // TODO: Could make this operation unchecked and assume the user size meets this criteria
    // for speed later
    size_t message_sz = sizeof(T);
    if (message_sz > MAX_MESSAGE_SIZE_BYTES)
    {
        throw std::invalid_argument("Message size is larger than the max message size");
    }

    MessageHeader header = {message_sz};

    char *start = shm_metadata_.shm_ptr + control_block_.write_offset;
    char *header_end = start + sizeof(MessageHeader);
    // Copy the header into the shared memory
    std::copy(reinterpret_cast<char *>(&header),
              reinterpret_cast<char *>(&header) + sizeof(MessageHeader), start);

    // Copy the message into the shared memory
    std::copy(reinterpret_cast<char *>(&message),
              reinterpret_cast<char *>(&message) + message_sz, header_end);

    control_block_.write_offset += sizeof(MessageHeader) + message_sz;
}

template <typename T>
T KoiQueue<T>::recv()
{
}

template <typename T>
size_t KoiQueue<T>::shm_size() const
{
    return shm_metadata_.shm_size;
}