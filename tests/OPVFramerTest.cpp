#include "OPVFramer.h"

#include <gtest/gtest.h>

#include <cstdint>

// make CXXFLAGS="$(pkg-config --cflags gtest) $(pkg-config --libs gtest) -I. -O3 -std=c++17" tests/ConvolutionTest

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class OPVFramerTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  // void TearDown() override {}
};

TEST_F(OPVFramerTest, construct)
{
    mobilinkd::OPVFramer framer;
}
