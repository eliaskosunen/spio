set(GSL_LITE_OPT_BUILD_TESTS OFF)
set(GSL_LITE_OPT_BUILD_EXAMPLES OFF)
add_subdirectory(gsl-lite EXCLUDE_FROM_ALL)

add_library(nonstd INTERFACE)
target_include_directories(nonstd INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(fmt EXCLUDE_FROM_ALL)

include(CheckCXXSourceCompiles)

set(CMAKE_REQUIRED_FLAGS -std=c++1z)
check_cxx_source_compiles("#include <filesystem>
int main() {
    auto path = std::filesystem::current_path();
    return 0;
}" has_std_filesystem)
if(NOT has_std_filesystem)
    set(has_std_filesystem OFF)
endif()

set(CMAKE_REQUIRED_FLAGS -std=c++1y)
set(CMAKE_REQUIRED_LIBRARIES stdc++fs)
check_cxx_source_compiles("#include <experimental/filesystem>
int main() {
    auto path = std::experimental::filesystem::current_path();
    return 0;
}" has_exp_filesystem)
if(NOT has_exp_filesystem)
    set(has_exp_filesystem OFF)
endif()

if(SPIO_USE_AFIO)
    if((NOT has_std_filesystem) AND (NOT has_exp_filesystem))
        message(SEND_ERROR "Afio requires std(::experimental)::filesystem")
    endif()

    add_subdirectory(afio)
    target_include_directories(afio_sl INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/afio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/afio/include/afio/v2.0)
    target_compile_features(afio_sl PUBLIC cxx_std_17)
    target_compile_options(afio_sl PRIVATE
        $<$<CXX_COMPILER_ID:GNU>: -w>)
endif()
