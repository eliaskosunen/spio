add_library(test-main INTERFACE)
target_link_libraries(test-main INTERFACE spio)
target_include_directories(test-main SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/doctest)
target_compile_options(test-main INTERFACE
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall -Wextra -pedantic
        -pedantic-errors -fvisibility=hidden
        -fstrict-aliasing -fdiagnostics-show-option
        -Wconversion -Wold-style-cast -Wfloat-equal
        -Wlogical-op -Wundef -Wredundant-decls
        -Wshadow -Wstrict-overflow=4 -Wwrite-strings
        -Wcast-qual -Wformat=2 -Wswitch-default
        -Wmissing-include-dirs -Wcast-align
        -Wswitch-enum -Wnon-virtual-dtor
        -Wctor-dtor-privacy -Wsign-conversion
        -Wdisabled-optimization
        -Winvalid-pch -Wmissing-declarations
        -Woverloaded-virtual
        $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,4.6>:
            -Wnoexcept>
        $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,5.0>:
            -Wdouble-promotion -Wtrampolines
            -Wzero-as-null-pointer-constant
            -Wuseless-cast -Wvector-operation-performance>
        $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,6.0>:
            -Wshift-overflow=2 -Wnull-dereference
            -Wduplicated-cond>
        $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,7.0>:
            -Walloc-zero -Walloca
            -Wduplicated-branches>
        $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,8.0>:
            -Wcast-align-strict=strict>
        >
    $<$<CXX_COMPILER_ID:Clang>:
        -Weverything
        -fvisibility=hidden -fstrict-aliasing
        -Wno-c++98-compat -Wno-c++98-compat-pedantic
        -Wno-padded -Wno-exit-time-destructors
        -Wno-weak-vtables
        >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /D_CRT_SECURE_NO_WARNINGS
        /D_SCL_SECURE_NO_WARNINGS
        /wd4514 # unreferenced inline function has removed
        /wd4571
        /wd4710 # function not inlined
        /wd4711 # function 'x' selected for automatic inline expansions
        /wd4616 # invalid compiler warning
        /wd4619 # invalid compiler warning
        /wd4820 # struct padding
        /wd4623 # implicitly deleted default ctor
        /wd4625 # implicitly deleted copy ctor
        /wd4626 # implicitly deleted copy assign
        /wd5026 # implicitly deleted move ctor
        /wd5027 # implicitly deleted move assing
        >)

message(STATUS "CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")

# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

function(print_properties)
    message ("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction(print_properties)

function(print_target_properties tgt)
    if(NOT TARGET ${tgt})
      message("There is no target named '${tgt}'")
      return()
    endif()

    foreach (prop ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
    # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
    if(prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
        continue()
    endif()
        # message ("Checking ${prop}")
        get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
        if (propval)
            get_target_property(propval ${tgt} ${prop})
            message ("${tgt} ${prop} = ${propval}")
        endif()
    endforeach(prop)
endfunction(print_target_properties)

function(add_spio_test name)
    add_executable(${name} ${name}.cpp ${ARGN})
    target_link_libraries(${name} test-main)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_spio_test(empty)
add_spio_test(stdio_device)
add_spio_test(ring)
add_spio_test(source_buffer)
add_spio_test(sink_buffer)
add_spio_test(filter)

print_target_properties(empty)

if(UNIX)
    add_executable(ring_std ring.cpp)
    target_link_libraries(ring_std test-main)
    target_compile_definitions(ring_std PRIVATE SPIO_RING_USE_MMAP=0)
    add_test(NAME ring_std COMMAND ring_std)

    add_executable(source_buffer_std source_buffer.cpp)
    target_link_libraries(source_buffer_std test-main)
    target_compile_definitions(source_buffer_std PRIVATE SPIO_RING_USE_MMAP=0)
    add_test(NAME source_buffer_std COMMAND source_buffer_std)
endif()
