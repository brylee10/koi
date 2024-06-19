#include "utils.hh"

void report_and_exit(const char *msg)
{
    perror(msg);
    exit(1);
}
