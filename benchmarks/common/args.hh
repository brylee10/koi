#pragma once

#include <getopt.h>
#include <iostream>

struct Args
{
    size_t message_size;
    unsigned long long iterations;
};

Args parse_args(int argc, char *argv[]);