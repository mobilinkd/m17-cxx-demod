#include "OPVRandomizer.h"

#include <gtest/gtest.h>

#include <cstdint>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class OPVRandomizerTest : public ::testing::Test {
 protected:
  void SetUp() override { }

  // void TearDown() override {}

};

TEST_F(OPVRandomizerTest, zero_bytes)
{
    std::array<uint8_t, 46> zeros;
    zeros.fill(0);
    mobilinkd::OPVByteRandomizer rnd;
    rnd(zeros);
    for (size_t i = 0; i != zeros.size(); ++i)
    {
        EXPECT_EQ(zeros[i], mobilinkd::detail::DC[i]);
    }
}

TEST_F(OPVRandomizerTest, one_bytes)
{
    std::array<uint8_t, 46> ones;
    ones.fill(0xFF);
    mobilinkd::OPVByteRandomizer rnd;
    rnd(ones);
    for (size_t i = 0; i != ones.size(); ++i)
    {
        EXPECT_EQ(ones[i], uint8_t(~mobilinkd::detail::DC[i]));
    }
}
