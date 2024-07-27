# cmake 3.0 released in June 2014
cmake_minimum_required(VERSION 3.0)
project(Koi LANGUAGES CXX VERSION 0.0.1)
# set(CMAKE_BUILD_TYPE Debug)

enable_testing()

# Set the mandatory C++ standard
set(CMAKE_CXX_STANDARD 23)
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
FetchContent_MakeAvailable(catch2)

# Add the test executable
# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#usage
find_package(Catch2 REQUIRED)

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
target_link_libraries(test_koi_queue PRIVATE Catch2::Catch2WithMain KoiQueue KoiCommonUtils)
target_include_directories(test_koi_queue PRIVATE 
    cpp/fixed_size/koi_queue 
    benchmarks/common 
    cpp/fixed_size/receiver cpp/fixed_size/sender tests
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
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

# Add Boost 1.84.0 (relatively recent, released Dec 13th 2023)
set(BOOST_URL "https://archives.boost.io/release/1.84.0/source/boost_1_84_0.tar.bz2" CACHE STRING "Boost download URL")
set(BOOST_URL_SHA256 "cc4b893acf645c9d4b698e9a0f08ca8846aa5d6c68275c14c3e7949c24109454" CACHE STRING "Boost download URL SHA256 checksum")
add_subdirectory(boost-cmake)

# Build libraries
add_library(KoiCommonUtils benchmarks/common/signals.cc cpp/common/logger.cc)
target_include_directories(KoiCommonUtils PUBLIC cpp/common)
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
# Koi
add_executable (koi_bench benchmarks/koi_queue/koi_benchmarks.cc)
target_link_libraries(koi_bench benchmark::benchmark KoiReceiver KoiSender)

# Boost IPC
add_executable (boost_ipc benchmarks/boost/boost_benchmarks.cc)
target_include_directories(boost_ipc PUBLIC benchmarks/boost)
target_link_libraries(boost_ipc benchmark::benchmark Boost::boost spdlog::spdlog)

add_executable(boost_test benchmarks/boost/boost_test.cc)
target_link_libraries(boost_test Boost::boost)

# Basic IPC benchmarks
add_library(BenchmarkUtils benchmarks/common/utils.cc 
    benchmarks/common/args.cc benchmarks/common/bench.cc
    benchmarks/common/signals.cc)
target_include_directories(BenchmarkUtils PUBLIC benchmarks/common)

add_library(ShmUtils benchmarks/shm/shm.cc) 
target_include_directories(ShmUtils PUBLIC benchmarks/shm benchmarks/common)

function(setup_ipc_executable target_name source output_name kind)
    add_executable(${target_name} ${source})
    target_link_libraries(${target_name} PRIVATE BenchmarkUtils ShmUtils)
    set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${kind} OUTPUT_NAME ${output_name})
endfunction()

setup_ipc_executable(shm_client benchmarks/shm/client.cc client shm)
setup_ipc_executable(shm_server benchmarks/shm/server.cc server shm)
setup_ipc_executable(launcher benchmarks/common/launcher.cc launcher launcher)