#include "args.hh"

Args parse_args(int argc, char *argv[])
{
    Args args;
    int opt;
    while ((opt = getopt(argc, argv, "m:i:")) != -1)
    {
        switch (opt)
        {
        case 'm':
            args.message_size = std::strtoul(optarg, nullptr, 10);
            break;
        case 'i':
            args.iterations = std::strtoull(optarg, nullptr, 10);
            break;
        default:
            std::cerr << "Usage: " << argv[0] << " -m <message_size> -i <iterations>\n";
            exit(EXIT_FAILURE);
        }
    }

    if (args.message_size == 0 || args.iterations == 0)
    {
        std::cerr << "Both message_size and iterations must be positive integers\n";
        exit(EXIT_FAILURE);
    }

    return args;
}