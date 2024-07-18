#pragma once

#include "koi_queue.hh"

// An IPC receiver
template <typename T>
class KoiReceiver : public KoiQueue<T>
{
public:
    KoiReceiver(const std::string_view name, size_t buffer_bytes) : KoiQueue<T>(name, buffer_bytes)
    {
    }

    using KoiQueue<T>::recv;
};