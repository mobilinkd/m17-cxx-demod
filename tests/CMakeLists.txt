include(GoogleTest)
pkg_check_modules(CODEC2 REQUIRED codec2)

include_directories (
    ${TEST_SOURCE_DIR}
    ..
)

add_executable (ConvolutionTest ConvolutionTest.cpp)
target_link_libraries(ConvolutionTest gtest pthread)
gtest_add_tests(ConvolutionTest "" AUTO)

add_executable (M17FramerTest M17FramerTest.cpp)
target_link_libraries(M17FramerTest gtest pthread)
gtest_add_tests(M17FramerTest "" AUTO)

add_executable (TrellisTest TrellisTest.cpp)
target_link_libraries(TrellisTest gtest pthread)
gtest_add_tests(TrellisTest "" AUTO)

add_executable (ViterbiTest ViterbiTest.cpp)
target_link_libraries(ViterbiTest gtest pthread)
gtest_add_tests(ViterbiTest "" AUTO)

add_executable (Golay24Test Golay24Test.cpp)
target_link_libraries(Golay24Test gtest pthread)
gtest_add_tests(Golay24Test "" AUTO)

add_executable (CRC16Test CRC16Test.cpp)
target_link_libraries(CRC16Test gtest pthread)
gtest_add_tests(CRC16Test "" AUTO)
