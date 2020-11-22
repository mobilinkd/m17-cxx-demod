cmake_minimum_required(VERSION 3.0)

project(m17-cxx-demod)
enable_language(CXX)

#C++11 is a required language feature for this project
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
    message(STATUS "Build type not specified: defaulting to release.")
endif()

include(FindPkgConfig)
include(GNUInstallDirs)

pkg_check_modules(CODEC2 REQUIRED codec2)

add_executable(m17-demod m17-demod.cpp)
target_link_libraries(m17-demod ${CODEC2_LIBRARIES})
install(TARGETS m17-demod DESTINATION ${CMAKE_INSTALL_BINDIR})
