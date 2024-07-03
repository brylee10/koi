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

    if (args.message_size > MAX_MESSAGE_SIZE)
    {
        std::cerr << "Message size must be less than " << MAX_MESSAGE_SIZE << "\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "Running server/client with args: message_size=" << args.message_size
              << ", iterations=" << args.iterations << std::endl;

    return args;
}

LauncherArgs parse_launcher_args(int argc, char *argv[])
{
    LauncherArgs args;
    int opt;
    while ((opt = getopt(argc, argv, "m:i:n:")) != -1)
    {
        switch (opt)
        {
        case 'm':
            args.message_size = std::strtoul(optarg, nullptr, 10);
            break;
        case 'i':
            args.iterations = std::strtoull(optarg, nullptr, 10);
            break;
        case 'n':
            args.benchmark_name = optarg;
            break;
        default:
            std::cerr << "Usage: " << argv[0] << " -m <message_size> -i <iterations> -n <benchmark name>" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    if (args.message_size == 0 || args.iterations == 0)
    {
        std::cerr << "Both message_size and iterations must be positive integers" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (args.message_size > MAX_MESSAGE_SIZE)
    {
        std::cerr << "Message size must be less than " << MAX_MESSAGE_SIZE << std::endl;
        exit(EXIT_FAILURE);
    }

    if (args.benchmark_name.empty())
    {
        std::cerr << "Benchmark name must be provided to the launcher" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Running the launcher with: message_size=" << args.message_size
              << ", iterations=" << args.iterations
              << ", benchmark_name=" << args.benchmark_name
              << std::endl;

    return args;
}