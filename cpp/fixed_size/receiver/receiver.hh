#pragma once

#include "koi_queue.hh"

// An IPC receiver
template <typename T>
class Receiver : KoiQueue<T>
{
public:
    Receiver(const std::string_view name, size_t buffer_bytes) : KoiQueue<T>(name, buffer_bytes)
    {
    }
};