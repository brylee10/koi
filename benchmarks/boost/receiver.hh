#pragma once
#include "utils.hh"

#include <optional>

template <typename T>
class Receiver
{
    IpcTransport<T> *transport;
    managed_shared_memory segment;
    const std::string name_;

public:
    Receiver(const std::string name) : name_(name)
    {
        segment = managed_shared_memory(open_only, name_.c_str());
        // Find the deque in the shared memory
        std::pair<IpcTransport<T> *, std::size_t> res = segment.find<IpcTransport<T>>("IpcTransport");
        transport = res.first;
    }

    std::optional<T> recv()
    {
        T val = transport->front();
        transport->pop_front();
        return val;
    }
};