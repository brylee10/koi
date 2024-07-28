#pragma once
#include "utils.hh"

#include "spdlog/spdlog.h"
#include <boost/circular_buffer.hpp>

template <typename T>
class BoundedBuffer
{
    enum Mode
    {
        Receiver,
        Sender,
    };

    IpcTransport<T> *container_;
    size_t unread_;
    managed_shared_memory segment_;
    const Mode mode_;
    const std::string name_;

public:
    // Constructor for Sender mode
    BoundedBuffer(const unsigned long queue_size, const std::string &name)
        : unread_(0), mode_(Sender), name_(name)
    {
        shared_memory_object::remove(name.c_str());

        spdlog::debug("Creating shared memory segment with {} number of bytes", queue_size);
        try
        {
            segment_ = managed_shared_memory(create_only, name.c_str(), queue_size * sizeof(T) + 65536);
            container_ = segment_.construct<IpcTransport<T>>("IpcTransport")(segment_.get_segment_manager());
            container_->set_capacity(queue_size);
            spdlog::debug("Transport created at address: {}", fmt::ptr(container_));
        }
        catch (const std::exception &e)
        {
            spdlog::error("Error: {}", e.what());
            throw;
        }
    }

    // Constructor for Receiver mode
    BoundedBuffer(const std::string &name)
        : unread_(0), mode_(Receiver), name_(name)
    {
        segment_ = managed_shared_memory(open_only, name.c_str());
        // Find the circular buffer in the shared memory
        std::pair<IpcTransport<T> *, std::size_t> res = segment_.find<IpcTransport<T>>("IpcTransport");
        container_ = res.first;
        if (!container_)
        {
            throw std::runtime_error("Failed to find circular buffer in shared memory");
        }
        spdlog::debug("Transport found at address: {}", fmt::ptr(container_));
    }

    ~BoundedBuffer()
    {
        shared_memory_object::remove(shared_memory_name.c_str());
    }

    void send(T val)
    {
        container_->push_back(val);
        spdlog::debug("After send, Size: {}, Capacity: {}", container_->size(), container_->capacity());
    }

    std::optional<T> recv()
    {
        T val = container_->front();
        container_->pop_front();
        return val;
    }

    size_t get_free_memory() const
    {
        return segment_.get_free_memory();
    }

    size_t size() const
    {
        return container_->size();
    }

    size_t capacity() const
    {
        return container_->capacity();
    }
};