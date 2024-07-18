#pragma once

#include "koi_queue.hh"

// An IPC sender
template <typename T>
class KoiSender : public KoiQueue<T>
{
public:
    KoiSender(const std::string_view name, size_t buffer_bytes) : KoiQueue<T>(name, buffer_bytes)
    {
    }

    using KoiQueue<T>::send;
};