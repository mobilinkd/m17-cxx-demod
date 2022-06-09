add_executable(opv-demod opv-demod.cpp)
target_link_libraries(opv-demod PRIVATE opvcxx codec2 opus Boost::program_options)

add_executable(opv-mod opv-mod.cpp)
target_link_libraries(opv-mod PRIVATE opvcxx codec2 opus Boost::program_options Threads::Threads)

install(TARGETS opv-demod opv-mod RUNTIME DESTINATION bin)
