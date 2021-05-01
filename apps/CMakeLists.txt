add_executable(m17-demod m17-demod.cpp)
target_link_libraries(m17-demod PRIVATE m17cxx ${CODEC2_LIBRARIES})

add_executable(m17-mod m17-mod.cpp)
target_link_libraries(m17-mod PRIVATE m17cxx ${CODEC2_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)

install(TARGETS m17-demod m17-mod RUNTIME DESTINATION bin)
