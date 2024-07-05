#pragma once

// An IPC receiver
template <typename T>
class Receiver
{
private:
    unsigned int size;
    T *data;

public:
    Receiver(unsigned int size) : size(size)
    {
        data = new T[size];
    }
};