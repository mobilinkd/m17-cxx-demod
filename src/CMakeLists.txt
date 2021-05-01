add_library(m17cxx INTERFACE)

target_include_directories(m17cxx INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/m17cxx>
    $<INSTALL_INTERFACE:include>
    )

target_compile_features(m17cxx INTERFACE cxx_std_17)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${HEADER_LIST})

install(TARGETS m17cxx
    EXPORT m17cxxTargets
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    RUNTIME DESTINATION bin COMPONENT Runtime
    PUBLIC_HEADER DESTINATION include COMPONENT Development
    BUNDLE DESTINATION bin COMPONENT Runtime
    INCLUDES DESTINATION include
    )

