enable_testing()

add_library(test-main INTERFACE)
target_link_libraries(test-main INTERFACE spio)
target_include_directories(test-main SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/doctest)
target_compile_options(test-main INTERFACE
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:
        -Weverything
        -Wno-c++98-compat -Wno-c++98-compat-pedantic
        -Wno-padded -Wno-exit-time-destructors
        -Wno-weak-vtables>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>)

function(add_spio_test name)
    add_executable(${name} ${name}.cpp ${ARGN})
    target_link_libraries(${name} test-main)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_spio_test(stdio_device)
add_spio_test(source_buffer)
add_spio_test(ring)
