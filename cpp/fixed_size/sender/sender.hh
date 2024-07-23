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
    // Currently only the sender is allowed to clean up the shared memory segment
    // since there is only one sender
    using KoiQueue<T>::cleanup_shm;
};