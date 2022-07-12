#include "PolynomialInterleaver.h"
#include "OPVRandomizer.h"

#include <gtest/gtest.h>

#include <cstdint>

using namespace mobilinkd;

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class PolynomialInterleaverTest : public ::testing::Test {
 protected:
  void SetUp() override { }

  // void TearDown() override {}

};

TEST_F(PolynomialInterleaverTest, byte_bit_interleaver)
{
    std::array<uint8_t, stream_type4_bytes> dc;
    std::copy(mobilinkd::detail::DC.begin(), mobilinkd::detail::DC.end(), dc.begin());

    std::array<int8_t, stream_type4_size> dc_bits;
    for (size_t i = 0; i != stream_type4_size; ++i)
    {
        dc_bits[i] = get_bit_index(dc, i);
    }
    PolynomialInterleaver interleaver;
    interleaver.interleave(dc_bits);
    interleaver.interleave(dc);
    for (size_t i = 0; i != stream_type4_size; ++i)
    {
        EXPECT_EQ(dc_bits[i], get_bit_index(dc, i));
    }
}

TEST_F(PolynomialInterleaverTest, reinterleave)
{
    std::array<uint8_t, stream_type4_bytes> dc;
    std::copy(mobilinkd::detail::DC.begin(), mobilinkd::detail::DC.end(), dc.begin());
    PolynomialInterleaver interleaver;
    interleaver.interleave(dc);
    interleaver.interleave(dc); // M17 interleaver is reversable.
    for (size_t i = 0; i != stream_type4_bytes; ++i)
    {
        EXPECT_EQ(dc[i], mobilinkd::detail::DC[i]);
    }
}

TEST_F(PolynomialInterleaverTest, deinterleave)
{
    std::array<uint8_t, stream_type4_bytes> dc;
    std::copy(mobilinkd::detail::DC.begin(), mobilinkd::detail::DC.end(), dc.begin());
    PolynomialInterleaver interleaver;
    interleaver.interleave(dc);
    interleaver.deinterleave(dc);
    for (size_t i = 0; i != stream_type4_bytes; ++i)
    {
        EXPECT_EQ(dc[i], mobilinkd::detail::DC[i]);
    }
}