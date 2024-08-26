// Boost bounded buffer using a circular buffer, implemented based on the Boost docs:
// https://www.boost.org/doc/libs/1_85_0/doc/html/circular_buffer/examples.html
#pragma once
#include "utils.hh"

#include "spdlog/spdlog.h"
#include <boost/circular_buffer.hpp>
#include <optional>

// Share the condition variables between threads
// Typically, the shared data would be passed into each
// thread on spawn but the Google Benchmark thread
// spawning does not allow this.
struct SharedData
{
    std::mutex mutex;
} shared_data;

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
    // Uses Boost Interprocess managed shared memory
    // https://www.boost.org/doc/libs/1_85_0/doc/html/interprocess/managed_memory_segments.html
    managed_shared_memory segment_;
    const Mode mode_;
    const std::string name_;

    bool is_empty() const
    {
        spdlog::debug("is_empty: Unread {}", container_->size());
        return container_->size() == 0;
    }
    bool full() const
    {
        spdlog::debug("full: Unread {}, Capacity {}", container_->size(), container_->capacity());
        return container_->full();
    }

public:
    // Queue size is measured in number of messages, not in bytes
    BoundedBuffer(const std::string &name, const unsigned long queue_size)
        : unread_(0), mode_(Sender), name_(name)
    {
        shared_memory_object::remove(name_.c_str());
        spdlog::debug("Create shared memory segment named {}", name_);
        try
        {
            segment_ = managed_shared_memory(open_or_create, name.c_str(), queue_size * sizeof(T) + 65536);
            // Find the circular buffer in the shared memory
            std::pair<IpcTransport<T> *, std::size_t> res = segment_.find<IpcTransport<T>>("IpcTransport");
            container_ = res.first;
            if (!container_)
            {
                // Initialize the circular buffer if not already present
                container_ = segment_.construct<IpcTransport<T>>("IpcTransport")(segment_.get_segment_manager());
                container_->set_capacity(queue_size);
                spdlog::debug("Transport created at address: {}", fmt::ptr(container_));
            }
            else
            {
                spdlog::debug("Transport opened at address: {}", fmt::ptr(container_));
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("Error: {}", e.what());
            throw;
        }
    }

    // Constructor which assumes the named shared memory segment already exists
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
        // spdlog::info("Removing shared memory segment named {}", name_);
        // shared_memory_object::remove(name_.c_str());
    }

    // Returns true if send succeeded, otherwise false
    bool send(T val)
    {
        std::unique_lock lock(shared_data.mutex);
        spdlog::debug("Sending a message");
        if (this->full())
        {
            spdlog::debug("Container full");
            // `unique_lock` unlocks
            return false;
        }
        container_->push_back(val);
        spdlog::debug("After send, Size: {}, Capacity: {}", container_->size(), container_->capacity());
        spdlog::debug("Done with sending message");
        return true;
    }

    std::optional<T> recv()
    {
        spdlog::debug("Running recv");
        std::unique_lock lock(shared_data.mutex);
        spdlog::debug("Receiving a message");
        if (this->is_empty())
        {
            spdlog::debug("Container empty");
            return std::nullopt;
        }
        T val = container_->front();
        spdlog::debug("Container size before pop: {}", container_->size());
        container_->pop_front();
        spdlog::debug("Container size after pop: {}", container_->size());
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