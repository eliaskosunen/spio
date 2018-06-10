add_subdirectory(third_party)

add_library(spio INTERFACE)
add_library(spio::spio ALIAS spio)
target_include_directories(spio INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(spio INTERFACE
    gsl-lite nonstd fmt-header-only)
if(SPIO_USE_AFIO)
    target_compile_features(spio INTERFACE cxx_std_14)
    target_compile_definitions(spio INTERFACE SPIO_USE_AFIO=1)
    target_link_libraries(spio INTERFACE afio_sl)
else()
    target_compile_features(spio INTERFACE cxx_defaulted_functions)
    target_compile_definitions(spio INTERFACE SPIO_USE_AFIO=0)
endif()
