#pragma once

#include "koi_queue.hh"

namespace koi
{
    // An IPC receiver
    template <typename T>
    class KoiReceiver : public KoiQueue<T>
    {
    public:
        KoiReceiver(const std::string name, size_t buffer_bytes) : KoiQueue<T>(name, buffer_bytes)
        {
        }

        using KoiQueue<T>::recv;
        using KoiQueue<T>::size;
    };
} // namespace koi