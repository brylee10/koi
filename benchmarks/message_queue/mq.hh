#pragma once

#include "utils.hh"
#include "types.hh"

#include <sys/ipc.h>
#include <sys/msg.h>
#include <cstdio>

constexpr ull MAX_MSG_SIZE = 8192; // Maximum message size for POSIX queues

// Creates a System V message queue, returning the message queue identifier
int create_mq()
{
    key_t mq_key;

    // Generates a deterministic key for a System V message queue
    mq_key = ftok("message_queue", 1);

    int msq_id = msgget(mq_key, IPC_CREAT);

    if (msq_id == -1)
    {
        perror("msgget");
        return 1;
    }

    return msq_id;
}