#include "Util.h"

#include <gtest/gtest.h>

#include <cstdint>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class UtilTest : public ::testing::Test {
 protected:
  void SetUp() override { }

  // void TearDown() override {}

};

/*
 * Bit index go from 0 to N for an array of M bytes, where N is
 * from M*8-7 to M*8.  Bits within bytes are accessed from high
 * bit to low bit as byte arrays are serialized MSB first.
 */
TEST_F(UtilTest, get_bit_index)
{
    using mobilinkd::get_bit_index;

    std::array<uint8_t, 1> data = {0x55};
    EXPECT_EQ(get_bit_index(data, 0), 0);
    EXPECT_EQ(get_bit_index(data, 1), 1);
    EXPECT_EQ(get_bit_index(data, 2), 0);
    EXPECT_EQ(get_bit_index(data, 3), 1);
    EXPECT_EQ(get_bit_index(data, 4), 0);
    EXPECT_EQ(get_bit_index(data, 5), 1);
    EXPECT_EQ(get_bit_index(data, 6), 0);
    EXPECT_EQ(get_bit_index(data, 7), 1);
}

TEST_F(UtilTest, set_bit_index)
{
    using mobilinkd::get_bit_index;
    using mobilinkd::set_bit_index;

    std::array<uint8_t, 1> data = {0x55};

    EXPECT_EQ(get_bit_index(data, 0), 0);

    set_bit_index(data, 0);

    EXPECT_EQ(get_bit_index(data, 0), 1);
    EXPECT_EQ(data[0], 0xD5);
}

TEST_F(UtilTest, reset_bit_index)
{
    using mobilinkd::get_bit_index;
    using mobilinkd::reset_bit_index;

    std::array<uint8_t, 1> data = {0x55};

    EXPECT_EQ(get_bit_index(data, 7), 1);

    reset_bit_index(data, 7);

    EXPECT_EQ(get_bit_index(data, 7), 0);
    EXPECT_EQ(data[0], 0x54);
}

TEST_F(UtilTest, assign_bit_index)
{
    using mobilinkd::get_bit_index;
    using mobilinkd::assign_bit_index;

    std::array<uint8_t, 1> data = {0x55};

    EXPECT_EQ(get_bit_index(data, 2), 0);
    EXPECT_EQ(get_bit_index(data, 5), 1);

    assign_bit_index(data, 2, 1);
    assign_bit_index(data, 5, 0);

    EXPECT_EQ(get_bit_index(data, 2), 1);
    EXPECT_EQ(get_bit_index(data, 5), 0);
    EXPECT_EQ(data[0], 0x71);
}

TEST_F(UtilTest, puncture_bytes)
{
    using mobilinkd::puncture_bytes;
    using mobilinkd::get_bit_index;

    std::array<uint8_t, 8> data = {0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55};
    std::array<int8_t, 8> PM = {1,1,1,1,1,1,1,0};
    std::array<uint8_t, 7> out;

    out.fill(0);
    auto result = puncture_bytes(data, out, PM);

    EXPECT_EQ(result, 56);
    EXPECT_EQ(get_bit_index(out, 48), 0);
    EXPECT_EQ(get_bit_index(out, 49), 0);
    EXPECT_EQ(get_bit_index(out, 50), 1);
    EXPECT_EQ(get_bit_index(out, 51), 0);
    EXPECT_EQ(get_bit_index(out, 52), 1);
    EXPECT_EQ(get_bit_index(out, 53), 0);
    EXPECT_EQ(get_bit_index(out, 54), 1);
    EXPECT_EQ(get_bit_index(out, 55), 0);
}