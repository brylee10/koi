#include "koi_queue.hh"

#include <iostream>

int main()
{
    [[maybe_unused]] KoiQueue queue = KoiQueue("koi_queue", static_cast<size_t>(1024));
    std::cout << "Hello!" << std::endl;
    return 0;
}
