#include "koi_queue.hh"
#include "test_utils.hh"
#include "signals.hh"

#include <catch2/catch_all.hpp>
#include <chrono>
#include <thread>

pid_t spawn_process()
{
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        return pid;
    }
    else if (pid > 0)
    {
        // Parent process
        return pid;
    }
    else
    {
        // Error
        perror("fork `spawn_process`");
        exit(EXIT_FAILURE);
    }
}

TEST_CASE("Single Send Recv", "[KoiQueue][MultiProcess]")
{
    const std::string shm_name = generate_unique_shm_name();
    struct Message
    {
        int x;
        int y;
    };

    Message msg = {1, 2};
    pid_t sender_pid = spawn_process();
    if (sender_pid == 0)
    {
        // Child process is sender
        KoiQueue<Message> queue(shm_name, SHM_SIZE);
        queue.send(msg);
        exit(EXIT_SUCCESS);
    }
    // Parent process is receiver
    // Wait for child to exit, indicating it is done sending
    if (waitpid(sender_pid, NULL, 0) == -1)
    {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    KoiQueue<Message> queue(shm_name, SHM_SIZE);
    auto recv_msg = queue.recv();
    REQUIRE(recv_msg.has_value());
    REQUIRE(recv_msg.value().x == msg.x);
    REQUIRE(recv_msg.value().y == msg.y);
}

TEST_CASE("Multiple Send Recv Alternating", "[KoiQueue][MultiProcess]")
{
    // Test sends `num_msg_groups` groups of `num_msgs` messages from sender to receiver
    // After each group, the sender waits until the receiver has exhausted the queue
    // before sending the next group of messages
    const std::string shm_name = generate_unique_shm_name();
    struct Message
    {
        unsigned int x;
        unsigned int y;
    };

    constexpr unsigned int num_msg_groups = 10;
    constexpr unsigned int num_msgs = 10;

    // Initialize array of messages to send on each iteration
    std::array<Message, num_msg_groups * num_msgs> msgs;
    for (unsigned int i = 0; i < num_msg_groups * num_msgs; ++i)
    {
        msgs[i] = {i, i};
    }
    SignalManager signal_manager_server = SignalManager(SignalManager::SignalTarget::SERVER);

    pid_t sender_pid = spawn_process();
    if (sender_pid == 0)
    {
        // Child process is receiver (client)
        SignalManager signal_manager_client = SignalManager(SignalManager::SignalTarget::CLIENT);
        KoiQueue<Message> queue(shm_name, SHM_SIZE);
        // Indicate client is ready to receive
        signal_manager_client.notify();
        for (unsigned int i = 0; i < num_msg_groups; ++i)
        {
            // Wait until server sends all messages
            signal_manager_client.wait_until_notify();
            for (unsigned int j = 0; j < num_msgs; ++j)
            {
                Message msg = msgs[i * num_msgs + j];
                auto recv_msg = queue.recv();
                REQUIRE(recv_msg.has_value());
                REQUIRE(recv_msg.value().x == msg.x);
                REQUIRE(recv_msg.value().y == msg.y);
            }
            // Indicate client is done reading all messages
            signal_manager_client.notify();
        }
        exit(EXIT_SUCCESS);
    }
    // Parent process is sender (server)
    // Wait until client is ready to receive
    signal_manager_server.wait_until_notify();
    KoiQueue<Message> queue(shm_name, SHM_SIZE);
    for (unsigned int i = 0; i < num_msg_groups; ++i)
    {
        for (unsigned int j = 0; j < num_msgs; ++j)
        {
            Message msg = msgs[i * num_msgs + j];
            REQUIRE(queue.send(msg) == KoiQueueRet::OK);
        }
        // Indicate server is done sending all messages
        signal_manager_server.notify();
        // Wait until client has read all messages
        signal_manager_server.wait_until_notify();
    }

    // Wait for child to exit, indicating it is done
    if (waitpid(sender_pid, NULL, 0) == -1)
    {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }
}

TEST_CASE("Multiple Send Recv Polling", "[KoiQueue][MultiProcess]")
{
    // This test case is similar to "KoiQueue Multiprocess Multiple Send Recv Alternating"
    // except the receiver will just continually poll the queue for messages until it has
    // read the expected number of messages
    const std::string shm_name = generate_unique_shm_name();
    struct Message
    {
        unsigned int x;
        unsigned int y;
    };

    constexpr unsigned int num_msg_groups = 10;
    constexpr unsigned int num_msgs = 10;
    // Timeout duration for polling
    // Each message should certainly be sent within 500ms
    constexpr std::chrono::milliseconds timeout_duration(500);

    // Initialize array of messages to send on each iteration
    std::array<Message, num_msg_groups * num_msgs> msgs;
    for (unsigned int i = 0; i < num_msg_groups * num_msgs; ++i)
    {
        msgs[i] = {i, i};
    }
    pid_t sender_pid = spawn_process();
    if (sender_pid == 0)
    {
        // Child process is receiver (client)
        KoiQueue<Message> queue(shm_name, SHM_SIZE);
        for (unsigned int i = 0; i < num_msg_groups * num_msgs; ++i)
        {
            auto start_time = std::chrono::steady_clock::now();
            Message msg = msgs[i];
            // Poll until message is received
            // Note, it is possible that this test case will hang
            // if the sender does not send
            // This should be timed out
            std::optional<Message> recv_msg;
            do
            {
                recv_msg = queue.recv();
                if (std::chrono::steady_clock::now() - start_time > timeout_duration)
                {
                    // Timeout error, fail test case
                    REQUIRE(false);
                }
            } while (!recv_msg.has_value());
            REQUIRE(recv_msg.has_value());
            REQUIRE(recv_msg.value().x == msg.x);
            REQUIRE(recv_msg.value().y == msg.y);
        }
        exit(EXIT_SUCCESS);
    }
    // Parent process is sender (server)
    KoiQueue<Message> queue(shm_name, SHM_SIZE);
    for (unsigned int i = 0; i < num_msg_groups * num_msgs; ++i)
    {
        Message msg = msgs[i];
        REQUIRE(queue.send(msg) == KoiQueueRet::OK);
    }

    // Wait for child to exit, indicating it is done
    if (waitpid(sender_pid, NULL, 0) == -1)
    {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }
}

TEST_CASE("Send Recv Two Full Queues", "[KoiQueue][MultiProcess]")
{
    // This test case is similar to the prior `[MultiProcess]` test cases
    // except this tests wrapping around the ring buffer and sender checking full queues.
    // The sender will send messages until it receives a full queue. The receiver will
    // read all messages from the queue and then this process will be repeated.
    const std::string shm_name = generate_unique_shm_name();
    struct Message
    {
        unsigned int x;
        unsigned int y;
    };

    constexpr unsigned int num_msgs_to_fill_queue = SHM_SIZE / CACHE_LINE_BYTES;
    constexpr unsigned int num_full_queues = 2;

    // Initialize array of messages to send on each iteration
    //
    std::array<Message, num_msgs_to_fill_queue * num_full_queues>
        msgs;
    for (unsigned int i = 0; i < num_msgs_to_fill_queue * num_full_queues; ++i)
    {
        msgs[i] = {i, i};
    }
    SignalManager signal_manager_server = SignalManager(SignalManager::SignalTarget::SERVER);

    pid_t sender_pid = spawn_process();
    if (sender_pid == 0)
    {
        // Child process is receiver (client)
        SignalManager signal_manager_client = SignalManager(SignalManager::SignalTarget::CLIENT);
        KoiQueue<Message> queue(shm_name, SHM_SIZE);
        // Indicate client is ready to receive
        signal_manager_client.notify();
        for (unsigned int i = 0; i < num_full_queues; ++i)
        {
            // Wait until server sends all messages
            signal_manager_client.wait_until_notify();
            for (unsigned int j = 0; j < num_msgs_to_fill_queue; ++j)
            {
                Message msg = msgs[i * num_msgs_to_fill_queue + j];
                auto recv_msg = queue.recv();
                REQUIRE(recv_msg.has_value());
                REQUIRE(recv_msg.value().x == msg.x);
                REQUIRE(recv_msg.value().y == msg.y);
            }
            // Indicate client is done reading all messages
            signal_manager_client.notify();
        }
        exit(EXIT_SUCCESS);
    }
    // Parent process is sender (server)
    // Wait until client is ready to receive
    signal_manager_server.wait_until_notify();
    KoiQueue<Message> queue(shm_name, SHM_SIZE);
    for (unsigned int i = 0; i < num_full_queues; ++i)
    {
        for (unsigned int j = 0; j < num_msgs_to_fill_queue; ++j)
        {
            Message msg = msgs[i * num_msgs_to_fill_queue + j];
            REQUIRE(queue.send(msg) == KoiQueueRet::OK);
        }
        // Queue should be full
        // REQUIRE(queue.send(msgs[0]) == KoiQueueRet::QUEUE_FULL);
        REQUIRE(queue.is_full());
        // Indicate server is done sending all messages
        signal_manager_server.notify();
        // Wait until client has read all messages
        signal_manager_server.wait_until_notify();
    }

    // Wait for child to exit, indicating it is done
    if (waitpid(sender_pid, NULL, 0) == -1)
    {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }
}