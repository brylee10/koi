#pragma once

#include "bench.hh"

template <typename T>
class Message
{
    ull len;
    T *data;

public:
    Message(ull len) : len(len);

    ~Message();
};