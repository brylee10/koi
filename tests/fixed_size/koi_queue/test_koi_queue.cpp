#define CATCH_CONFIG_MAIN
#include "koi_queue.hh"

// No longer <catch2/catch.hpp> since v3
// https://github.com/catchorg/Catch2/blob/devel/docs/migrate-v2-to-v3.md#how-to-migrate-projects-from-v2-to-v3
#include <catch2/catch_all.hpp>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <unistd.h>

// MacOS M1 has a page size of 16KB
constexpr size_t PAGE_SIZE = 1 << 14;
constexpr size_t SHM_SIZE_PAGES = 1 << 4;
constexpr size_t SHM_SIZE = PAGE_SIZE * SHM_SIZE_PAGES;
const std::string SHM_NAME = "koi_queue";

TEST_CASE("KoiQueue Lifecycle", "[KoiQueue]")
{
    SECTION("Initialization")
    {
        KoiQueue<char> queue(SHM_NAME, SHM_SIZE);
        queue.init_shm();

        // Sanity check: ensure the shared memory is correctly initialized
        struct stat sb;
        int shm_fd = shm_open("koi_queue", O_RDONLY, 0666);
        REQUIRE(shm_fd != -1);
        fstat(shm_fd, &sb);
        close(shm_fd);

        // This will match as long as `SHM_SIZE` is a multiple of the page size
        REQUIRE(sb.st_size == static_cast<off_t>(SHM_SIZE));
    }

    SECTION("Destructor")
    {
        {
            KoiQueue<char> queue(SHM_NAME, SHM_SIZE);
            queue.init_shm();
        } // Destructor called here

        // Ensure the shared memory was unlinked
        int shm_fd = shm_open(SHM_NAME.c_str(), O_RDONLY, 0666);
        REQUIRE(shm_fd == -1);
        REQUIRE(errno == ENOENT);
    }
}

TEST_CASE("KoiQueue Sanity Checks", "[KoiQueue]")
{
    SECTION("Minimum SHM size")
    {
        REQUIRE_THROWS_AS(KoiQueue<char>(SHM_NAME, sizeof(ControlBlock) - 1), std::invalid_argument);
    }

    SECTION("Byte buffer rounding")
    {
        // Ensure the buffer size is rounded up to the nearest cache line size
        KoiQueue<char> queue(SHM_NAME, CACHE_LINE_BYTES - 1);
        REQUIRE(queue.shm_size() == CACHE_LINE_BYTES);
    }
}