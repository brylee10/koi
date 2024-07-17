#pragma once

#include "koi_queue.hh"

// An IPC sender
template <typename T>
class Sender : KoiQueue<T>
{
public:
    Sender(const std::string_view name, size_t buffer_bytes) : KoiQueue<T>(name, buffer_bytes)
    {
    }
};