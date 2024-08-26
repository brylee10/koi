#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/circular_buffer.hpp>
#include <iostream>
#include <string>
#include <cassert>

using namespace boost::interprocess;

// Even with no elements the transport needs space for metadata
// Through some searching, the transport requires ~280 bytes of overhead.
// Allocate extra to be conservative.
constexpr size_t TRANSPORT_OVERHEAD_BYTES = 512;

// Define an allocator to allocate elements in the shared memory
template <typename T>
using ShmemAllocator = allocator<T, managed_shared_memory::segment_manager>;

// Define a deque that uses the allocator
template <typename T>
using IpcTransport = boost::circular_buffer<T, ShmemAllocator<T>>;
