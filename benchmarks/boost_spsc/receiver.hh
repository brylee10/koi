#pragma once
#include <boost/lockfree/spsc_queue.hpp>

template <typename T>
class Receiver
{
    std::unique_ptr<boost::lockfree::spsc_queue<T>> transport;

public:
    Receiver(const unsigned long queue_size) : transport(std::make_unique<boost::lockfree::spsc_queue<T>>(queue_size))
    {
    }

    std::optional<T> recv()
    {
        T val;
        if (transport->pop(val))
        {
            return val;
        }
        return std::nullopt;
    }
};