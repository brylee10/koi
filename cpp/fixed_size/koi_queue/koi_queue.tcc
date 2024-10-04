
#include "koi_queue.hh"
#include "koi_utils.hh"

#include "spdlog/spdlog.h"

#include <iostream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <unistd.h>

// Named constants for `open_shm` return values for semantic typing
constexpr int SHM_CREATED = 0;
constexpr int SHM_EXISTS = 1;

// If the shm segment at `shm_name` has already been created, then the provided `user_shm_size` must be
// the same as the existing shared memory size and the `message_block_sz_` must be the same as the
// existing shared memory message block size encoded in the shared memory control block.
template <typename T>
KoiQueue<T>::KoiQueue(const std::string shm_name, size_t user_shm_size)
{
    load_spdlog_level();
    // `send`/`recv` will do a bitwise memcpy of T into the shared memory
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
    static_assert(message_sz <= MAX_MESSAGE_SIZE_BYTES, "Message size is larger than the max message size");

    // TODO: Add check that the message size is not larger than the shm size

    spdlog::info("Constructing KoiQueue with shm_name: {}, user_shm_size: {} bytes", shm_name, user_shm_size);
    spdlog::info("KoiQueue running with message_sz: {}, header size: {} bytes, message_block_sz: {} bytes", message_sz, sizeof(MessageHeader), message_block_sz_);
    shm_metadata_.shm_name = std::move(shm_name);

    // The `user_shm_size` must be a power of 2
    if ((user_shm_size & (user_shm_size - 1)) != 0)
    {
        throw std::invalid_argument("user_shm_size provided " + std::to_string(user_shm_size) +
                                    " is not a power of 2");
    }
    shm_metadata_.user_shm_size = user_shm_size;

    int open_ret = -1;
    try
    {
        open_ret = open_shm();
        init_shm(open_ret);
    }
    catch (const std::exception &e)
    {
        // Destructor will not be called. Clean up the shared memory file, if any
        cleanup_shm();
        throw;
    }

    ControlBlock *control_block = reinterpret_cast<ControlBlock *>(shm_metadata_.shm_ptr);
    control_block_ = control_block;
    if (open_ret == SHM_EXISTS)
    {
        // The shared memory already exists, so the control block is already initialized
        // Read the control block from the shared memory
        // Sanity check that the user_shm_size is the same as the existing shared memory
        // The `write.user_shm_size` is arbitrarily selected. The `read` shm size would have worked equally well.
        if (control_block_->write.user_shm_size != user_shm_size)
        {
            spdlog::error("user_shm_size provided: {}, existing user_shm_size: {}", user_shm_size, control_block_->write.user_shm_size);
            throw std::runtime_error("user_shm_size provided does not match existing shared memory");
        }

        // Sanity check that the message block size is the same as the existing shared memory
        if (control_block_->write.message_block_sz != message_block_sz_)
        {
            spdlog::error("message_block_sz_ provided: {}, existing message_block_sz: {}",
                          message_block_sz_, control_block_->write.message_block_sz);
            throw std::runtime_error("message_block_sz_ provided does not match existing shared memory");
        }
        return;
    }
    // The shared memory was created, so initialize the control block
    control_block_->write.user_shm_size = user_shm_size;
    control_block_->write.message_block_sz = message_block_sz_;
    control_block_->write.offset = 0;
    control_block_->read.user_shm_size = user_shm_size;
    control_block_->read.message_block_sz = message_block_sz_;
    control_block_->read.offset = 0;
    spdlog::debug("Control block initialized with user_shm_size: {}, message_block_sz: {}",
                  control_block_->write.user_shm_size, control_block_->write.message_block_sz);
}

template <typename T>
KoiQueue<T>::~KoiQueue()
{
    spdlog::debug("Starting KoiQueue destructor");
    // Previously cleaned up the shm in the destructor, but this
    // is not done to:
    // 1) avoid a dropped receiver cleaning up memory while a sender is using it
    // 2) allow introspection of the shared memory segment after the sender/receiver has finished
    // 3) allow subsequent senders/receivers to use the same shared memory segment after prior
    //    participants have finished
    // cleanup_shm();
}

template <typename T>
void KoiQueue<T>::cleanup_shm() noexcept
{
    spdlog::debug("Cleaning up shared memory");
    if (shm_unlink(shm_metadata_.shm_name.c_str()) == -1)
    {
        // `shm_unlink` can fail if the shared memory was already unlinked by the client/server
    }

    // Can only unmap if the shared memory was successfully mapped
    // This should be called after `shm_unlink` because the `mmap` occurs after
    // the `shm_open` call in `init_shm`.
    if (shm_metadata_.shm_ptr != nullptr)
    {
        if (munmap(shm_metadata_.shm_ptr, shm_metadata_.total_shm_size) == -1)
        {
            // munmap can fail if the constructor threw an exception
            // before the shared memory was mapped
            // spdlog::error("munmap failed");
        }
    }
}

// Initialization order is `open_shm` then `init_shm`
// If open is successful, returns:
// - `SHM_CREATED` if the shared memory was created
// - `SHM_EXISTS` if the shared memory already exists
// - Throws an error otherwise
template <typename T>
int KoiQueue<T>::open_shm()
{
    spdlog::debug("Checking if shared memory already exists");
    int shm_status = SHM_CREATED;
    int shm_fd = shm_open(shm_metadata_.shm_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        if (errno == EEXIST)
        {
            spdlog::debug("Shared memory already exists, opening existing shared memory");
            shm_fd = shm_open(shm_metadata_.shm_name.c_str(), O_RDWR, 0660);
            if (shm_fd == -1)
            {
                perror("shm_open");
                throw std::runtime_error("Failed to open shared memory");
            }
            shm_status = SHM_EXISTS;
        }
        else
        {
            spdlog::error("Failed to create shared memory");
            perror("shm_open");
            throw std::runtime_error("Failed to create shared memory");
        }
    }
    else
    {
        spdlog::debug("Shared memory did not exist");
    }
    shm_metadata_.shm_fd = shm_fd;
    return shm_status;
}

// Returns 0 on success, throws an error otherwise
template <typename T>
int KoiQueue<T>::init_shm(int shm_status)
{
    // The shared memory is initialized with extra space bytes which holds the control block
    size_t control_block_sz = size_rounded_to_cache_line<ControlBlock>();
    size_t total_shm_size = control_block_sz + shm_metadata_.user_shm_size;
    spdlog::debug("ftruncate with total_shm_size: {}", total_shm_size);
    if (ftruncate(shm_metadata_.shm_fd, total_shm_size) == -1)
    {
        // `ftruncate` on an already open file descriptor can fail with EINVAL
        // https://stackoverflow.com/questions/20320742/ftruncate-failed-at-the-second-time
        if (errno != EINVAL)
        {
            spdlog::error("ftruncate failed with errno: {}", errno);
            spdlog::error("fd: {}, total_shm_size: {}", shm_metadata_.shm_fd, total_shm_size);
            perror("ftruncate");
            throw std::runtime_error("ftruncate failed");
        }
    }

    char *shm_ptr = static_cast<char *>(mmap(NULL, total_shm_size,
                                             PROT_READ | PROT_WRITE, MAP_SHARED, shm_metadata_.shm_fd, 0));
    if (shm_ptr == MAP_FAILED)
    {
        perror("mmap");
        throw std::runtime_error("Failed to map shared memory");
    }
    shm_metadata_.shm_ptr = shm_ptr;
    // The queue shared memory starts after the control block
    shm_metadata_.user_shm_start = shm_ptr + control_block_sz;
    shm_metadata_.total_shm_size = total_shm_size;

    if (shm_status == SHM_CREATED)
    {
        spdlog::debug("Resetting all occupied flags in the message headers");
        // Initialize all message headers to unoccupied on first creating the shm segment
        for (size_t i = 0; i < shm_metadata_.user_shm_size; i += message_block_sz_)
        {
            MessageHeader *header = reinterpret_cast<MessageHeader *>(shm_metadata_.user_shm_start + i);
            header->occupied = false;
        }
    }
    spdlog::debug("Finished init_shm");
    return 0;
}

template <typename T>
KoiQueueRet KoiQueue<T>::send(T message)
{
    char *start = shm_metadata_.user_shm_start + control_block_->write.offset;
    MessageHeader *header = reinterpret_cast<MessageHeader *>(start);
    if (header->occupied)
    {
        return KoiQueueRet::QUEUE_FULL;
    }

    // Copy the message into the shared memory
    char *header_end = start + sizeof(MessageHeader);
    std::copy(reinterpret_cast<char *>(&message),
              reinterpret_cast<char *>(&message) + message_sz, header_end);

    // Every message is rounded up to the nearest cache line (`message_block_sz_`)
    // Wrap around the ring buffer
    // The following three control block fields are all on the same cacheline
    control_block_->write.offset = (control_block_->write.offset + control_block_->write.message_block_sz) & (control_block_->write.user_shm_size - 1);
    header->occupied = true;
    return KoiQueueRet::OK;
}

template <typename T>
std::optional<T> KoiQueue<T>::recv()
{
    char *start = shm_metadata_.user_shm_start + control_block_->read.offset;
    MessageHeader *header = reinterpret_cast<MessageHeader *>(start);
    if (!header->occupied)
    {
        return std::nullopt;
    }

    char *header_end = start + sizeof(MessageHeader);
    T message;
    std::copy(header_end, header_end + message_sz, reinterpret_cast<char *>(&message));

    // Every message is rounded up to the nearest cache line (`message_block_sz_`)
    // Wrap around the ring buffer
    // The following three control block fields are all on the same cacheline
    control_block_->read.offset = (control_block_->read.offset + control_block_->read.message_block_sz) & (control_block_->read.user_shm_size - 1);
    header->occupied = false;
    return message;
}

template <typename T>
size_t KoiQueue<T>::user_shm_size() const
{
    return shm_metadata_.user_shm_size;
}

template <typename T>
size_t KoiQueue<T>::curr_queue_sz_bytes() const
{
    size_t curr_queue_sz = (control_block_->write.offset -
                            control_block_->read.offset + control_block_->write.user_shm_size) &
                           (control_block_->read.user_shm_size - 1);
    if (curr_queue_sz == 0 && is_full())
    {
        // Special case where the queue is full and the read and write offsets are the same
        return control_block_->write.user_shm_size;
    }
    return curr_queue_sz;
}

template <typename T>
size_t KoiQueue<T>::shm_remaining_bytes() const
{
    // spdlog::debug("user_shm_size: {}, curr_queue_sz_bytes: {}", control_block_->write.user_shm_size, curr_queue_sz_bytes());
    return control_block_->write.user_shm_size - curr_queue_sz_bytes();
}

template <typename T>
constexpr size_t KoiQueue<T>::message_block_sz_bytes() const
{
    return message_block_sz_;
}

template <typename T>
bool KoiQueue<T>::is_full() const
{
    char *start = shm_metadata_.user_shm_start + control_block_->write.offset;
    MessageHeader *header = reinterpret_cast<MessageHeader *>(start);
    return header->occupied;
}

template <typename T>
bool KoiQueue<T>::is_empty() const
{
    char *start = shm_metadata_.user_shm_start + control_block_->read.offset;
    MessageHeader *header = reinterpret_cast<MessageHeader *>(start);
    return !header->occupied;
}

template <typename T>
size_t KoiQueue<T>::size() const
{
    return curr_queue_sz_bytes() / message_block_sz_;
}

template <typename T>
size_t KoiQueue<T>::capacity() const
{
    spdlog::debug("user_shm_size: {}, message_block_sz_: {}", control_block_->write.user_shm_size, message_block_sz_);
    return control_block_->write.user_shm_size / message_block_sz_;
}