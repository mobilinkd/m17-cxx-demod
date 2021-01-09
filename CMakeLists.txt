cmake_minimum_required(VERSION 3.9)

project(m17-cxx-demod)
enable_language(CXX)

# C++17 is a required language feature for this project
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
    message(STATUS "Build type not specified: defaulting to release.")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(FindPkgConfig)
include(GNUInstallDirs)

pkg_check_modules(CODEC2 REQUIRED codec2)
SET( Boost_USE_STATIC_LIBS FALSE )
FIND_PACKAGE( Boost COMPONENTS program_options REQUIRED)

add_executable(m17-demod m17-demod.cpp)
target_link_libraries(m17-demod ${CODEC2_LIBRARIES})
install(TARGETS m17-demod DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(m17-mod m17-mod.cpp)
target_link_libraries(m17-mod PRIVATE ${CODEC2_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(m17-mod PRIVATE Threads::Threads)
install(TARGETS m17-mod DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing ()
add_subdirectory (tests)
