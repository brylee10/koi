#pragma once

#include "utils.hh"
#include "bounded_buffer.hh"

#include "spdlog/spdlog.h"
#include <cassert>

namespace boost_lock_buffer
{
    template <typename T>
    class Sender
    {
        std::unique_ptr<BoundedBuffer<T>> transport;
        const std::string name_;

    public:
        Sender(const std::string name, const size_t queue_size) : name_(name)
        {
            transport = std::make_unique<BoundedBuffer<T>>(name, queue_size);
        }

        bool send(T val)
        {
            return transport->send(val);
        }

        size_t get_free_memory() const
        {
            return transport->get_free_memory();
        }

        size_t size() const
        {
            spdlog::debug("Sender size: {}", transport->size());
            return transport->size();
        }

        size_t capacity() const
        {
            return transport->capacity();
        }
    };
} // namespace boost_lock_buffer