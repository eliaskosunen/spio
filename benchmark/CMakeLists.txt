set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Build Google Benchmark tests")
add_subdirectory(google-benchmark)

add_executable(bench bench_main.cpp bench_print.cpp)
target_link_libraries(bench PUBLIC test-main benchmark)
target_include_directories(test-main SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/google-benchmark)
target_compile_options(bench PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:
        -Wno-global-constructors
        -Wno-used-but-marked-unused
        -Wno-covered-switch-default
        >)
