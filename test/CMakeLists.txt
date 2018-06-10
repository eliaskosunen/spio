enable_testing()

add_executable(test test.cpp)
target_link_libraries(test spio)
target_compile_options(test PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall -Wextra -Wpedantic
        -Wno-sign-conversion>
    $<$<CXX_COMPILER_ID:Clang>:
        -Weverything
        -Wno-c++98-compat -Wno-c++98-compat-pedantic
        -Wno-padded -Wno-exit-time-destructors
        -Wno-sign-conversion -Wno-weak-vtables>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>)
