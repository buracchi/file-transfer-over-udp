add_library(ftcp INTERFACE)
target_include_directories(ftcp INTERFACE "include")
target_compile_definitions(ftcp INTERFACE FTCP_VERSION=1)
