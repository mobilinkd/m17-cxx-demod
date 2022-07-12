include(GoogleTest)

include_directories (
    ${TEST_SOURCE_DIR}
    ..
)

if(WIN32)
	set(PTHREAD "")
else()
	set(PTHREAD "pthread")
endif(WIN32)

add_executable (ConvolutionTest ConvolutionTest.cpp)
target_link_libraries(ConvolutionTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(ConvolutionTest "" AUTO)

add_executable (OPVFramerTest OPVFramerTest.cpp)
target_link_libraries(OPVFramerTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(OPVFramerTest "" AUTO)

add_executable (TrellisTest TrellisTest.cpp)
target_link_libraries(TrellisTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(TrellisTest "" AUTO)

add_executable (ViterbiTest ViterbiTest.cpp)
target_link_libraries(ViterbiTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(ViterbiTest "" AUTO)

add_executable (Golay24Test Golay24Test.cpp)
target_link_libraries(Golay24Test opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(Golay24Test "" AUTO)

add_executable (CRC16Test CRC16Test.cpp)
target_link_libraries(CRC16Test opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(CRC16Test "" AUTO)

add_executable (OPVRandomizerTest OPVRandomizerTest.cpp)
target_link_libraries(OPVRandomizerTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(OPVRandomizerTest "" AUTO)

add_executable (PolynomialInterleaverTest PolynomialInterleaverTest.cpp)
target_link_libraries(PolynomialInterleaverTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(PolynomialInterleaverTest "" AUTO)

add_executable (UtilTest UtilTest.cpp)
target_link_libraries(UtilTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(UtilTest "" AUTO)

add_executable (FrameHeaderTest FrameHeaderTest.cpp)
target_link_libraries(FrameHeaderTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(FrameHeaderTest "" AUTO)

add_executable (SlidingDFTTest SlidingDFTTest.cpp)
target_link_libraries(SlidingDFTTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(SlidingDFTTest "" AUTO)

add_executable (DataCarrierDetectTest DataCarrierDetectTest.cpp)
target_link_libraries(DataCarrierDetectTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(DataCarrierDetectTest "" AUTO)

add_executable (ClockRecoveryTest ClockRecoveryTest.cpp)
target_link_libraries(ClockRecoveryTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(ClockRecoveryTest "" AUTO)

add_executable (FreqDevEstimatorTest FreqDevEstimatorTest.cpp)
target_link_libraries(FreqDevEstimatorTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(FreqDevEstimatorTest "" AUTO)

add_executable (CorrelatorTest CorrelatorTest.cpp)
target_link_libraries(CorrelatorTest opvcxx GTest::GTest ${PTHREAD})
gtest_add_tests(CorrelatorTest "" AUTO)
