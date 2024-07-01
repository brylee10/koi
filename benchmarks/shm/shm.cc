#include "shm.hh"
#include "args.hh"

#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <algorithm>
#include <string_view>
#include <cassert>
#include <stdexcept>

ShmManager::ShmManager(const Args &args, const std::string_view shm_name)
    : shm_ptr(nullptr), shm_size(args.message_size * SHM_NUM_MSG),
      message_size(args.message_size), shm_name(shm_name)
{
}

// Does not throw exceptions, only logs errors
// https://stackoverflow.com/questions/130117/if-you-shouldnt-throw-exceptions-in-a-destructor-how-do-you-handle-errors-in-i
ShmManager::~ShmManager()
{
    std::cout << "Destroying ShmManager" << std::endl;
    // "If one or more references to the shared memory object exist when the object is unlinked,
    // the name shall be removed before shm_unlink() returns, but the removal of the memory
    // object contents shall be postponed until all open and map references to the shared memory
    // object have been removed."
    // https://pubs.opengroup.org/onlinepubs/009695399/functions/shm_unlink.html
    if (shm_unlink(shm_name.data()) == -1)
    {
        std::cerr << "shm_unlink failed" << std::endl;
    }

    // Can only unmap if the shared memory was successfully mapped
    // This should be called after `shm_unlink` because the `mmap` occurs after
    // the `shm_open` call in `init_shm`.
    if (shm_ptr != nullptr)
    {
        if (munmap(shm_ptr, shm_size) == -1)
        {
            std::cerr << "munmap failed" << std::endl;
        }
    }
}

// This is not in the constructor so the constructor does not throw exceptions, which would
// cause object construction to be incomplete and the destructor would not be called.
void ShmManager::init_shm()
{
    int fd = shm_open(shm_name.data(), O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        std::cerr << "errno: " << errno << std::endl;
        perror("shm_open");
        throw std::runtime_error("shm_open failed");
    }
    if (ftruncate(fd, shm_size) == -1)
    {
        // `ftruncate` on an already open file descriptor can fail with EINVAL
        // https://stackoverflow.com/questions/20320742/ftruncate-failed-at-the-second-time
        if (errno != EINVAL)
        {
            std::cerr << "errno: " << errno << std::endl;
            std::cout << "fd: " << fd << ", shm_size: " << shm_size << std::endl;
            perror("ftruncate");
            throw std::runtime_error("ftruncate failed");
        }
    }
    shm_ptr = (char *)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        std::cerr << "errno: " << errno << std::endl;
        perror("mmap");
        throw std::runtime_error("mmap failed");
    }
    // "After the mmap() call has returned, the file descriptor, fd, can
    // be closed immediately without invalidating the mapping."
    // https://man7.org/linux/man-pages/man2/mmap.2.html
    close(fd);
}

void ShmManager::write_shm(const std::string_view message)
{
#if COMPILE_ASSERTS
    assert(message.size() == message_size);
#endif
    std::copy(message.begin(), message.end(), shm_ptr);
}

std::string_view ShmManager::read_shm()
{
    return std::string_view(shm_ptr, message_size);
}