add_library(
        ftcp
        STATIC
        "src/ftcp.c"
)
target_compile_definitions(ftcp PRIVATE FTCP_VERSION=1)
target_include_directories(ftcp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

