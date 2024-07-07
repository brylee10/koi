#define CATCH_CONFIG_MAIN
#include "koi_queue.hh"

// No longer <catch2/catch.hpp> since v3
// https://github.com/catchorg/Catch2/blob/devel/docs/migrate-v2-to-v3.md#how-to-migrate-projects-from-v2-to-v3
#include <catch2/catch_all.hpp>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <unistd.h>

// Platform specific
// MacOS M1 has a page size of 16KB
// `getconf PAGE_SIZE` to check
constexpr size_t PAGE_SIZE = 1 << 14;
constexpr size_t SHM_SIZE_PAGES = 1 << 1;
constexpr size_t SHM_SIZE = PAGE_SIZE * SHM_SIZE_PAGES;
const std::string SHM_NAME = "koi_queue";

TEST_CASE("KoiQueue Lifecycle", "[KoiQueue]")
{
    SECTION("Initialization")
    {
        KoiQueueRAII<char> queue(SHM_NAME, SHM_SIZE);

        // Sanity check: ensure the shared memory is correctly initialized
        struct stat sb;
        int shm_fd = shm_open(SHM_NAME.c_str(), O_RDONLY, 0666);
        REQUIRE(shm_fd != -1);
        fstat(shm_fd, &sb);
        close(shm_fd);

        // This will match as long as `expected_shm_size` is a multiple of the page size
        // Shared memory size from st_size is `SHM_SIZE + CACHE_LINE_BYTES` rounded up to
        // the nearest PAGE_SIZE, where CACHE_LINE_BYTES is the size allocated to the control block
        size_t expected_shm_size = (SHM_SIZE + CACHE_LINE_BYTES + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        REQUIRE(sb.st_size == static_cast<off_t>(expected_shm_size));
    }

    SECTION("Destructor")
    {
        {
            KoiQueueRAII<char> queue(SHM_NAME, SHM_SIZE);
        } // Destructor called here

        // Ensure the shared memory was unlinked
        int shm_fd = shm_open(SHM_NAME.c_str(), O_RDONLY, 0666);
        REQUIRE(shm_fd == -1);
        REQUIRE(errno == ENOENT);
    }
}

TEST_CASE("KoiQueue Send Recv", "[KoiQueue]")
{
    struct Message
    {
        int x;
        int y;
    };

    SECTION("Send Recv Single")
    {
        KoiQueueRAII<Message> queue(SHM_NAME, SHM_SIZE);

        Message msg = {1, 2};
        (*queue).send(msg);

        auto recv_msg = (*queue).recv();
        REQUIRE(recv_msg.has_value());
        REQUIRE(recv_msg.value().x == 1);
        REQUIRE(recv_msg.value().y == 2);
    }

    SECTION("Send Recv Multiple")
    {
        KoiQueueRAII<Message> queue(SHM_NAME, SHM_SIZE);

        std::vector<Message> msgs;
        for (int i = 0; i < 10; ++i)
        {
            msgs.push_back({i, i});
            (*queue).send(msgs.back());
        }

        for (int i = 0; i < 10; ++i)
        {
            auto recv_msg = (*queue).recv();
            REQUIRE(recv_msg.has_value());
            REQUIRE(recv_msg.value().x == msgs[i].x);
            REQUIRE(recv_msg.value().y == msgs[i].y);
        }
    }
}

TEST_CASE("KoiQueue Error Handling", "[KoiQueue]")
{
    SECTION("Queue Full")
    {
        KoiQueueRAII<int> queue(SHM_NAME, SHM_SIZE);

        REQUIRE((*queue).shm_remaining_bytes() == SHM_SIZE);

        // Do a few send and recvs first
        for (int i = 0; i < 5; ++i)
        {
            (*queue).send(i);
            auto recv_msg = (*queue).recv();
            REQUIRE(recv_msg.has_value());
            REQUIRE(recv_msg.value() == i);
        }

        REQUIRE((*queue).shm_remaining_bytes() == SHM_SIZE);

        // Fill the queue
        size_t max_queue_messages = SHM_SIZE / (*queue).message_block_sz_bytes();
        for (int i = 0; i < max_queue_messages; ++i)
        {
            (*queue).send(i);
            REQUIRE((*queue).shm_remaining_bytes() == SHM_SIZE - (i + 1) * (*queue).message_block_sz_bytes());
        }

        // The sent messages should exactly fill the queue
        REQUIRE((*queue).shm_remaining_bytes() == 0);

        // Attempt to send to a full queue
        REQUIRE((*queue).send(0) == KoiQueueRet::QUEUE_FULL);
    }

    // The below would not compile because the KoiQueue constructor statically checks
    // the message size is <= MAX_MESSAGE_SIZE_BYTES
    // SECTION("Send Message Size")
    // {
    //     // Check `send` throws an exception if the message size is too large
    //     KoiQueue<char[MAX_MESSAGE_SIZE_BYTES + 1]> queue(SHM_NAME, SHM_SIZE);
    //     queue.init_shm();
    // }
}

TEST_CASE("KoiQueue Sanity Checks", "[KoiQueue]")
{
    SECTION("Minimum SHM size")
    {
        // Ensure the shared memory size is at least the size of the control block
        REQUIRE_THROWS_AS(KoiQueueRAII<char>(SHM_NAME, sizeof(ControlBlock) - 1), std::invalid_argument);
    }

    SECTION("Message block size rounding")
    {
        // Test message block size is the `sizeof(T)` rounded up to the nearest cache line size
        KoiQueueRAII<char> queue(SHM_NAME, SHM_SIZE);
        REQUIRE((*queue).message_block_sz_bytes() == CACHE_LINE_BYTES);
    }

    SECTION("Valid byte buffer sizes")
    {
        // Check a buffer size that is not a power of 2 throws an exception
        REQUIRE_THROWS_AS(KoiQueueRAII<char>(SHM_NAME, CACHE_LINE_BYTES - 1), std::invalid_argument);
        REQUIRE_THROWS_AS(KoiQueueRAII<char>(SHM_NAME, CACHE_LINE_BYTES * 6), std::invalid_argument);
    }
}