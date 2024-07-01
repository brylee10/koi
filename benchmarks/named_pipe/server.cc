#include "args.hh"
#include "named_pipe.hh"
#include "bench.hh"
#include "types.hh"

void start_server(Args args)
{
    FifoManager fifo_s2c(server_to_client_fifo, args);
    FifoManager fifo_c2s(client_to_server_fifo, args);
    Benchmarks benchmarks(std::string("named_pipe"), args.message_size);

    for (ull i = 0; i < args.iterations; i++)
    {
        // Full ping pong
        benchmarks.start_iteration();
        fifo_s2c.write_fifo();
        fifo_c2s.read_fifo();
        benchmarks.end_iteration(1);
    }
}

int main(int argc, char *argv[])
{
    Args args = parse_args(argc, argv);
    start_server(args);

    return 0;
}