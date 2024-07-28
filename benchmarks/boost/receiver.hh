#pragma once
#include "utils.hh"
#include "bounded_buffer.hh"

#include <optional>

template <typename T>
class Receiver
{
    std::unique_ptr<BoundedBuffer<T>> transport;
    const std::string name_;

public:
    Receiver(const std::string name) : name_(name)
    {
        transport = std::make_unique<BoundedBuffer<T>>(name);
    }

    std::optional<T> recv()
    {
        return transport->recv();
    }
};