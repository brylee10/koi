#include "message.hh"

template <typename T>
class Message
{
    ull len;
    T *data;

public:
    Message(ull len) : len(len)
    {
        data = new T[len];
    }

    ~Message()
    {
        delete[] data;
    }
};