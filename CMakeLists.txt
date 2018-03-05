cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(qpack VERSION 0.10.7)

option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)

if (NOT DEFINED CMAKE_BUILD_TYPE)
	set (CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_DEBUG_POSTFIX _d)

configure_file(
	"${CMAKE_SOURCE_DIR}/qpack.h.in"
	"${CMAKE_SOURCE_DIR}/qpack.h"
)

set(qpack_header ${CMAKE_SOURCE_DIR}/qpack.h)

add_library(qpack STATIC ${CMAKE_SOURCE_DIR}/qpack.c)
target_include_directories(qpack PRIVATE ${CMAKE_SOURCE_DIR})
set_target_properties(qpack PROPERTIES SUFFIX ".so")

if (BUILD_EXAMPLES)
	add_executable(example ${CMAKE_SOURCE_DIR}/example/main.c)
	target_include_directories(example PRIVATE ${CMAKE_SOURCE_DIR})
	target_link_libraries(example PRIVATE qpack)
endif()

if (BUILD_TESTS)
	add_executable(test_ ${CMAKE_SOURCE_DIR}/test/main.c)
	target_include_directories(test_ PRIVATE ${CMAKE_SOURCE_DIR})
	target_link_libraries(test_ PRIVATE qpack)
endif()

install(
	TARGETS qpack 
	EXPORT qpack-targets
	ARCHIVE DESTINATION lib
)

install(
	FILES ${qpack_header}
	DESTINATION include
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CONF_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/cmake/${CMAKE_PROJECT_NAME}")

write_basic_package_version_file(
	"${CMAKE_BINARY_DIR}/qpack-config-version.cmake"
	COMPATIBILITY AnyNewerVersion
)

export(EXPORT qpack-targets FILE ${CMAKE_BINARY_DIR}/qpack-targets.cmake)

configure_package_config_file(
	"${CMAKE_SOURCE_DIR}/qpack-config.cmake.in"
	"${CMAKE_BINARY_DIR}/qpack-config.cmake"
	INSTALL_DESTINATION ${CONF_INSTALL_DIR}
	PATH_VARS CMAKE_INSTALL_FULL_INCLUDEDIR CMAKE_INSTALL_FULL_LIBDIR
)

install(EXPORT qpack-targets FILE qpack-targets.cmake DESTINATION ${CONF_INSTALL_DIR})

install(FILES ${CMAKE_BINARY_DIR}/qpack-config.cmake ${CMAKE_BINARY_DIR}/qpack-config-version.cmake DESTINATION ${CONF_INSTALL_DIR})
