cmake_minimum_required(VERSION 3.9)

project(opvcxx
    VERSION 0.1
    DESCRIPTION "OPV Digital Voice modulation and demodulation"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
    message(STATUS "Build type not specified: defaulting to release.")
endif()

# Check for dependencies
message(STATUS "# Checking dependencies")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(GNUInstallDirs)

#find_package(Opus REQUIRED)
set(Boost_USE_STATIC_LIBS FALSE)
find_package(Boost COMPONENTS program_options REQUIRED)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(apps)

find_package(GTest)
if(GTEST_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()

# Setup installation
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/opvcxxConfigVersion.cmake"
    VERSION 0.1
    COMPATIBILITY AnyNewerVersion
    )

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/opvcxxConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/opvcxxConfig.cmake"
    INSTALL_DESTINATION lib/cmake/opvcxx
    )

install(EXPORT opvcxxTargets DESTINATION lib/cmake/opvcxx)
install(FILES "${PROJECT_BINARY_DIR}/opvcxxConfigVersion.cmake"
              "${PROJECT_BINARY_DIR}/opvcxxConfig.cmake"
        DESTINATION lib/cmake/opvcxx)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)

# Create uninstall target
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    @ONLY)

add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )

# Print summary
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building version: ${PROJECT_VERSION}")
