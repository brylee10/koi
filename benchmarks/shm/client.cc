#include "shm.hh"
#include "args.hh"
#include "signals.hh"

#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */

int main(int argc, char *argv[])
{
    std::cout << "Launching client" << std::endl;
    try
    {
        Args args = parse_args(argc, argv);
        ShmManager shm_s2c(args, SHM_NAME_S2C);
        shm_s2c.init_shm();
        ShmManager shm_c2s(args, SHM_NAME_C2S);
        shm_c2s.init_shm();

        SignalManager signal_manager(SignalManager::SignalTarget::CLIENT);

        // Indicate to server client is ready
        signal_manager.notify();
        for (ull i = 0; i < args.iterations; i++)
        {
            // signal_manager.wait_until_notify();
            const std::string_view server_msg = shm_s2c.read_shm();
            shm_c2s.write_shm(server_msg);
            // Notify server that client has read message
        }
        signal_manager.notify();
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