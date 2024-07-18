# cmake 3.0 released in June 2014
cmake_minimum_required(VERSION 3.0)
project(Koi LANGUAGES CXX VERSION 0.0.1)

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
  GIT_TAG v1.14.0 # Specify the version or branch you want to use
)

# Make the spdlog content available
FetchContent_MakeAvailable(spdlog)

# Build libraries
add_library(KoiCommonUtils benchmarks/common/signals.cc cpp/common/logger.cc)
target_include_directories(KoiCommonUtils PUBLIC cpp/common)
target_link_libraries(KoiCommonUtils PUBLIC spdlog::spdlog)

add_library(KoiQueue INTERFACE)
target_include_directories(KoiQueue INTERFACE cpp/fixed_size/koi_queue cpp/common)
target_link_libraries(KoiQueue INTERFACE KoiCommonUtils)

add_library(KoiReceiver INTERFACE)
target_link_libraries(KoiReceiver INTERFACE KoiQueue)