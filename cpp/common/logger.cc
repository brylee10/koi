#include "logger.hh"

void load_spdlog_level()
{
    // SPDLOG_LEVEL=info,mylogger=trace && ./executable
    spdlog::cfg::load_env_levels();
}