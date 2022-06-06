cmake_minimum_required(VERSION 3.9)

project(opv-example
    VERSION 0.1
    DESCRIPTION "Standalone example OPV project"
    LANGUAGES CXX)

find_package(opvcxx CONFIG REQUIRED)
if(${opvcxx_FOUND})
    message(STATUS "Found opvcxx")
    message(STATUS "  OPV Library version: " ${opvcxx_VERSION})
    message(STATUS "  OPV Library location: " ${opvcxx_DIR})
else(${opvcxx_FOUND})
    message(FATAL_ERROR "Could not locate opvcxx")
endif()

add_executable(example example.cpp)
target_link_libraries(example opvcxx)
