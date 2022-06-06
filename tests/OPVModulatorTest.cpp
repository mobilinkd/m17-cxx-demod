#include "OPVModulator.h"

#include <gtest/gtest.h>

#include <cstdint>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class OPVModulatorTest : public ::testing::Test {
 protected:
  void SetUp() override { }

  // void TearDown() override {}

};

TEST_F(OPVModulatorTest, construct)
{
    std::array<int16_t, 320> zeros;
    zeros.fill(0);
    mobilinkd::OPVModulator mod("W1AW");
}

TEST_F(OPVModulatorTest, run)
{
    using namespace mobilinkd;

    std::array<int16_t, 320> zeros;
    zeros.fill(0);
    OPVModulator mod("W1AW");
    auto audio_queue = std::make_shared<OPVModulator::audio_queue_t>();
    auto bitstream_queue = std::make_shared<OPVModulator::bitstream_queue_t>();
    auto future = mod.run(audio_queue, bitstream_queue);
    ASSERT_EQ(mod.state(), OPVModulator::State::IDLE);
    audio_queue->close();
    future.get();
    bitstream_queue->close();
}

