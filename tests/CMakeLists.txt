include(GoogleTest)
pkg_check_modules(CODEC2 REQUIRED codec2)

include_directories (
    ${TEST_SOURCE_DIR}
    ..
)

add_executable (ConvolutionTest ConvolutionTest.cpp)
target_link_libraries(ConvolutionTest gtest)
gtest_add_tests(ConvolutionTest "" AUTO)

add_executable (M17FramerTest M17FramerTest.cpp)
target_link_libraries(M17FramerTest gtest)
gtest_add_tests(M17FramerTest "" AUTO)

add_executable (TrellisTest TrellisTest.cpp)
target_link_libraries(TrellisTest gtest)
gtest_add_tests(TrellisTest "" AUTO)

add_executable (ViterbiTest ViterbiTest.cpp)
target_link_libraries(ViterbiTest gtest)
gtest_add_tests(ViterbiTest "" AUTO)

add_executable (Golay24Test Golay24Test.cpp)
target_link_libraries(Golay24Test gtest)
gtest_add_tests(Golay24Test "" AUTO)
