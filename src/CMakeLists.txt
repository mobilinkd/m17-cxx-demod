add_library(opvcxx INTERFACE)

target_include_directories(opvcxx INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/opvcxx>
    $<INSTALL_INTERFACE:include>
    )

target_compile_features(opvcxx INTERFACE cxx_std_20)

if(MSVC)
    # specify standards-conformance mode
    target_compile_options(opvcxx INTERFACE /permissive-)
    target_compile_definitions(opvcxx INTERFACE _USE_MATH_DEFINES)
endif()

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${HEADER_LIST})

install(TARGETS opvcxx
    EXPORT opvcxxTargets
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    RUNTIME DESTINATION bin COMPONENT Runtime
    PUBLIC_HEADER DESTINATION include COMPONENT Development
    BUNDLE DESTINATION bin COMPONENT Runtime
    INCLUDES DESTINATION include
    )

