#pragma once

#include <spdlog/spdlog.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace boost_spsc
{
    template <typename T>
    class Receiver
    {
        boost::lockfree::spsc_queue<T> *transport;
        managed_shared_memory segment_;
        size_t queue_size_;

    public:
        Receiver(const std::string_view name, const size_t queue_size) : queue_size_(queue_size)
        {
            spdlog::debug("Constructing Boost SPSC Receiver");
            // Allocate a `queue_size` number of elements plus overhead
            // String view does not guarantee null character termination so it has no c_str() like std::string
            // The original string will have null termination, so `data()` is used
            segment_ = managed_shared_memory(open_or_create, name.data(), queue_size * sizeof(T) + 65536);
            // When `spsc_queue` size is not determined at compile time, element count must be provided in constructor
            // See: https://www.boost.org/doc/libs/1_76_0/doc/html/boost/lockfree/spsc_queue.html
            // `explicit spsc_queue(size_type element_count);`
            transport = segment_.find_or_construct<boost::lockfree::spsc_queue<T>>("IpcTransport")(queue_size);
        }

        // Returns the received message, or `std::nullopt` if no message was received
        std::optional<T> recv()
        {
            spdlog::debug("Receiving a message");
            T val;
            if (transport->pop(val))
            {
                return val;
            }
            return std::nullopt;
        }

        // Returns number of elements in the queue
        size_t size() const
        {
            spdlog::debug("Number of elements in queue: {}", transport->read_available());
            return transport->read_available();
        }

        // Returns the capacity of the queue
        size_t capacity() const
        {
            return queue_size_;
        }
    };
} // namespace boost_spsc