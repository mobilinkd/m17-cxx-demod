#include "FreqDevEstimator.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class FreqDevEstimatorTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  // void TearDown() override {}
};

TEST_F(FreqDevEstimatorTest, construct)
{
    auto fde = mobilinkd::FreqDevEstimator<float>();
}

TEST_F(FreqDevEstimatorTest, fde_preamble)
{
    auto fde = mobilinkd::FreqDevEstimator<float>();
    fde.update(-3, 3);
    fde.update(-3, 3);
    fde.update(-3, 3);

    EXPECT_NEAR(fde.deviation(), 2400, .1);
    EXPECT_NEAR(fde.error(), 0, .1);
}
