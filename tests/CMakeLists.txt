enable_testing()

add_executable(tests test.cpp)
target_link_libraries(tests spio)
target_compile_options(tests PRIVATE
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
