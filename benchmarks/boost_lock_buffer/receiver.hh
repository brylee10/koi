#pragma once
#include "utils.hh"
#include "bounded_buffer.hh"

#include <optional>

namespace boost_lock_buffer
{
    template <typename T>
    class Receiver
    {
        std::unique_ptr<BoundedBuffer<T>> transport;
        const std::string name_;

    public:
        Receiver(const std::string name, const unsigned long queue_size) : name_(name)
        {
            transport = std::make_unique<BoundedBuffer<T>>(name, queue_size);
        }

        Receiver(const std::string name) : name_(name)
        {
            transport = std::make_unique<BoundedBuffer<T>>(name);
        }

        std::optional<T> recv()
        {
            return transport->recv();
        }

        size_t size() const
        {
            return transport->size();
        }
    };
} // namespace boost_lock_buffer