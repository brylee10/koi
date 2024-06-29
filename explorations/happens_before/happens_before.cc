#include <stdio.h>
#include <pthread.h>
#include <iostream>

// Shared variable
const char *s;

void *thread1_func(void *arg)
{
    puts(s);
    s = "Hello, world!";

    return nullptr;
}

int main()
{
    s = "hello";

    pthread_t t;

    // Thread 2
    if (pthread_create(&t, NULL, thread1_func, NULL) != 0)
    {
        std::cerr << "pthread_create failed" << std::endl;
        return 1;
    }

    // Join thread 2
    if (pthread_join(t, NULL) != 0)
    {
        std::cerr << "pthread_join failed" << std::endl;
        return 1;
    }

    // Thread 1
    puts(s);

    return 0;
}