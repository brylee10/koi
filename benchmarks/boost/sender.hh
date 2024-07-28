#pragma once

#include "utils.hh"
#include "bounded_buffer.hh"

#include "spdlog/spdlog.h"
#include <cassert>

template <typename T>
class Sender
{
    std::unique_ptr<BoundedBuffer<T>> transport;
    const std::string name_;

public:
    Sender(const unsigned long queue_size, const std::string name) : name_(name)
    {
        transport = std::make_unique<BoundedBuffer<T>>(queue_size, name);
    }

    void send(T val)
    {
        transport->send(val);
        spdlog::debug("After send, Size: {}, Capacity: {}", transport->size(), transport->capacity());
    }

    size_t get_free_memory() const
    {
        return transport->get_free_memory();
    }
};