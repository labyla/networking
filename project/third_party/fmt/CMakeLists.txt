add_library(fmt STATIC
    format.cpp
    os.cpp
)

target_include_directories(fmt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(fmt PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)
