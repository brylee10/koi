cmake_minimum_required(VERSION 3.14)
project(Koi LANGUAGES CXX VERSION 0.0.1)

enable_testing()

# Set the mandatory C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Get Catch2 via FetchContent
# from: https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#cmake-targets
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0 # or a later release
)
FetchContent_MakeAvailable(Catch2)

# Disable the Google Benchmark requirement on Google Test
set(BENCHMARK_ENABLE_TESTING NO)

# Fetch Google benchmark from its GitHub repository
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG origin/main
)

# Make the spdlog content available
FetchContent_MakeAvailable(googlebenchmark)

add_executable(test_koi_queue 
    tests/fixed_size/koi_queue/test_single_thread.cpp 
    tests/fixed_size/koi_queue/test_multiprocess.cpp
)
target_link_libraries(test_koi_queue PRIVATE Catch2::Catch2WithMain KoiQueue)
target_include_directories(test_koi_queue PRIVATE 
    cpp/fixed_size/koi_queue 
    benchmarks/common 
    cpp/fixed_size/receiver cpp/fixed_size/sender tests
)

set_target_properties(test_koi_queue PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test
    OUTPUT_NAME "koi_fixed_size"
)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(test_koi_queue)

# Fetch spdlog from its GitHub repository
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.0
)

# Make the spdlog content available
FetchContent_MakeAvailable(spdlog)

# Add Boost 1.71.0 (released Apr 7th 2021, per https://boostorg.jfrog.io/artifactory/main/release/)
# Did not select more recent Boost 1.86.0 (Aug 8 2024) because boost-cmake is pinned to 1.71.0
set(BOOST_URL "https://boostorg.jfrog.io/artifactory/main/release/1.71.0/source/boost_1_71_0.tar.bz2" CACHE STRING "Boost download URL")
set(BOOST_URL_SHA256 "d73a8da01e8bf8c7eda40b4c84915071a8c8a0df4a6734537ddde4a8580524ee" CACHE STRING "Boost download URL SHA256 checksum")
# See instructions on adding boost-cmake: https://github.com/Orphis/boost-cmake
add_subdirectory(boost-cmake)

# Build libraries
add_library(KoiCommonUtils benchmarks/common/signals.cc cpp/common/koi_utils.cc benchmarks/common/utils.cc)
target_include_directories(KoiCommonUtils PUBLIC cpp/common benchmarks/common)
target_link_libraries(KoiCommonUtils PUBLIC spdlog::spdlog)

add_library(KoiQueue INTERFACE)
target_include_directories(KoiQueue INTERFACE cpp/fixed_size/koi_queue cpp/common)
target_link_libraries(KoiQueue INTERFACE KoiCommonUtils)

add_library(KoiReceiver INTERFACE)
target_include_directories(KoiReceiver INTERFACE cpp/fixed_size/receiver)
target_link_libraries(KoiReceiver INTERFACE KoiQueue)

add_library(KoiSender INTERFACE)
target_include_directories(KoiSender INTERFACE cpp/fixed_size/sender)
target_link_libraries(KoiSender INTERFACE KoiQueue)

# Benchmarks
# Memcpy baseline
add_executable (memcpy benchmarks/memcpy/memcpy.cc)
target_link_libraries(memcpy benchmark::benchmark KoiCommonUtils)
set_target_properties(memcpy PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/benchmarks
    OUTPUT_NAME "memcpy"
)

# Shared SPSC benchmark
add_executable (spsc_benchmarks benchmarks/spsc_benchmarks.cc)
target_include_directories(spsc_benchmarks PUBLIC cpp benchmarks)
target_link_libraries(spsc_benchmarks benchmark::benchmark Boost::boost KoiReceiver KoiSender KoiQueue)
set_target_properties(spsc_benchmarks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/benchmarks
    OUTPUT_NAME "spsc_benchmarks"
)
