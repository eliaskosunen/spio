cmake_minimum_required(VERSION 3.2.3)

project(spio LANGUAGES CXX VERSION 0.1.0)

set(SPIO_USE_AFIO OFF CACHE BOOL "Use afio for vectored (scatter/gather) IO. Requires C++14 and std::filesystem")
message(STATUS "Use afio: ${SPIO_USE_AFIO}")

set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_subdirectory(include/spio)
add_subdirectory(tests)
