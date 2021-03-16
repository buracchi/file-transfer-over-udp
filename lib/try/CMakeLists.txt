add_library(
	try
	INTERFACE
)

target_include_directories(try INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_definitions(try INTERFACE TRY_VERSION=1)
