include(GoogleTest)
pkg_check_modules(CODEC2 REQUIRED codec2)

include_directories (
    ${TEST_SOURCE_DIR}
    ..
)

add_executable (ConvolutionTest ConvolutionTest.cpp)
target_link_libraries(ConvolutionTest m17cxx gtest pthread)
gtest_add_tests(ConvolutionTest "" AUTO)

add_executable (M17FramerTest M17FramerTest.cpp)
target_link_libraries(M17FramerTest m17cxx gtest pthread)
gtest_add_tests(M17FramerTest "" AUTO)

add_executable (TrellisTest TrellisTest.cpp)
target_link_libraries(TrellisTest m17cxx gtest pthread)
gtest_add_tests(TrellisTest "" AUTO)

add_executable (ViterbiTest ViterbiTest.cpp)
target_link_libraries(ViterbiTest m17cxx gtest pthread)
gtest_add_tests(ViterbiTest "" AUTO)

add_executable (Golay24Test Golay24Test.cpp)
target_link_libraries(Golay24Test m17cxx gtest pthread)
gtest_add_tests(Golay24Test "" AUTO)

add_executable (CRC16Test CRC16Test.cpp)
target_link_libraries(CRC16Test m17cxx gtest pthread)
gtest_add_tests(CRC16Test "" AUTO)

add_executable (M17RandomizerTest M17RandomizerTest.cpp)
target_link_libraries(M17RandomizerTest m17cxx gtest pthread)
gtest_add_tests(M17RandomizerTest "" AUTO)

add_executable (PolynomialInterleaverTest PolynomialInterleaverTest.cpp)
target_link_libraries(PolynomialInterleaverTest m17cxx gtest pthread)
gtest_add_tests(PolynomialInterleaverTest "" AUTO)

add_executable (M17ModulatorTest M17ModulatorTest.cpp)
target_link_libraries(M17ModulatorTest m17cxx gtest pthread codec2)
gtest_add_tests(M17ModulatorTest "" AUTO)

add_executable (UtilTest UtilTest.cpp)
target_link_libraries(UtilTest m17cxx gtest pthread)
gtest_add_tests(UtilTest "" AUTO)

add_executable (LinkSetupFrameTest LinkSetupFrameTest.cpp)
target_link_libraries(LinkSetupFrameTest m17cxx gtest pthread)
gtest_add_tests(LinkSetupFrameTest "" AUTO)
