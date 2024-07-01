#include "shm.hh"
#include "args.hh"
#include "signals.hh"
#include "bench.hh"

#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */

int main(int argc, char *argv[])
{
    try
    {
        Args args = parse_args(argc, argv);
        std::cout << "Launching server" << std::endl;
        SignalManager signal_manager(SignalManager::SignalTarget::SERVER);
        Benchmarks benchmarks(std::string("shm"), args.message_size);

        ShmManager shm_s2c(args, SHM_NAME_S2C);
        shm_s2c.init_shm();
        ShmManager shm_c2s(args, SHM_NAME_C2S);
        shm_c2s.init_shm();

        // Allocate string to write
        std::string message(args.message_size, 'a');
        std::cout << "Server message: " << message << std::endl;

        // Wait until client notifies that it is ready
        signal_manager.wait_until_notify();
        for (ull i = 0; i < args.iterations; i++)
        {
            benchmarks.start_iteration();
            shm_s2c.write_shm(message);
            // Notify client that server has written message
            // signal_manager.notify();
            // Wait until client is done reading
            const std::string_view client_msg = shm_c2s.read_shm();
            if (client_msg != message)
            {
                std::cerr << "Client message mismatch: " << client_msg << std::endl;
            }
            benchmarks.end_iteration(1);
        }
        signal_manager.wait_until_notify();
        return 0;
    }
    catch (const std::exception &e)
    {
        // Catch exceptions to allow for graceful exit and stack unwinding to
        // run destructors.
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}