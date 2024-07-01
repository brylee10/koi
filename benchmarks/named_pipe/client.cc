#include "args.hh"
#include "named_pipe.hh"
#include "bench.hh"
#include "types.hh"

void start_server(Args args)
{
    FifoManager fifo_s2c(server_to_client_fifo, args);
    FifoManager fifo_c2s(client_to_server_fifo, args);
    for (ull i = 0; i < args.iterations; i++)
    {
        // Full ping pong
        fifo_c2s.write_fifo();
        fifo_s2c.read_fifo();
    }
}

int main(int argc, char *argv[])
{
    Args args = parse_args(argc, argv);
    start_server(args);

    return 0;
}