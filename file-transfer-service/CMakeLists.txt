find_package(buracchi-common CONFIG REQUIRED)

add_library(file-transfer-service STATIC "src/fts.c")
target_include_directories(file-transfer-service PUBLIC "include")
target_link_libraries(file-transfer-service PUBLIC buracchi::common::networking)
target_link_libraries(file-transfer-service PRIVATE buracchi::common::logger)
target_link_libraries(file-transfer-service PRIVATE buracchi::common::concurrency)
target_link_libraries(file-transfer-service PRIVATE buracchi::common::utilities)
target_link_libraries(file-transfer-service INTERFACE coverage_config)

if (BUILD_TESTING)
	add_subdirectory("test")
endif ()
