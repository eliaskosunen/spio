add_subdirectory(third_party)

add_library(spio INTERFACE)
add_library(spio::spio ALIAS spio)
target_include_directories(spio INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(spio INTERFACE
    gsl-lite nonstd fmt-header-only)
if(SPIO_USE_LLFIO)
    target_compile_features(spio INTERFACE cxx_std_14)
    target_compile_definitions(spio INTERFACE SPIO_USE_LLFIO=1)
    target_link_libraries(spio INTERFACE llfio_sl)
else()
    target_compile_features(spio INTERFACE
        cxx_defaulted_functions
        cxx_noexcept
        cxx_variadic_templates
        cxx_nullptr
        cxx_reference_qualified_functions
        cxx_inline_namespaces)
    target_compile_definitions(spio INTERFACE SPIO_USE_LLFIO=0)
endif()
