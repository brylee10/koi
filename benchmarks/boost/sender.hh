#pragma once

#include <cassert>
#include "utils.hh"
#include "spdlog/spdlog.h"

template <typename T>
class Sender
{
    IpcTransport<T> *transport;
    managed_shared_memory segment;
    const std::string name_;

public:
    Sender(const unsigned long queue_size, const std::string name) : name_(name)
    {
        // Check that `queue_size` is at least 2x the message size. Otherwise the queue would
        // not be able to hold a message. Otherwise raise an exception.
        if (queue_size < 2 * sizeof(T))
        {
            throw std::invalid_argument("queue_size must be at least 2x the message size");
        }

        // Remove the shared memory object if it exists
        shared_memory_object::remove(name_.c_str());

        spdlog::debug("Creating shared memory segment with {} number of bytes", queue_size);
        try
        {
            segment = managed_shared_memory(create_only, name_.c_str(), queue_size);
            transport = segment.construct<IpcTransport<T>>("IpcTransport")(segment.get_segment_manager());
            spdlog::debug("Transport created at address: {}", fmt::ptr(transport));
            // Number of elements the queue can hold given the queue_size limit
            size_t capacity = (queue_size - TRANSPORT_OVERHEAD_BYTES) / sizeof(T);
            spdlog::debug("Trying to allocate capacity (considering overhead): {}", capacity);
            transport->set_capacity(capacity);
            spdlog::debug("Transport has capacity: {}", transport->capacity());
        }
        catch (const std::exception &e)
        {
            spdlog::error("Error: {}", e.what());
            throw;
        }
    }

    ~Sender()
    {
        shared_memory_object::remove(shared_memory_name.c_str());
    }

    void send(T val)
    {
        transport->push_back(val);
        spdlog::debug("After send, Size: {}, Capacity: {}", transport->size(), transport->capacity());
    }

    size_t get_free_memory() const
    {
        return segment.get_free_memory();
    }
};