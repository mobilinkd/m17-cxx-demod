#pragma once

#include <array>

namespace mobilinkd
{
    const int opus_bitrate = 16000;         // target output bit rate from encoder
    const int encoded_plheader_size = 240;  // Rate 1/2 Golay coded, 15 byte plheader
    const int punctured_payload_size = 1184;  // Encoded and 11/12 punctured, 2 * 320 bit codec payload + 3 for byte alignment
    const int frame_size_bits = encoded_plheader_size + punctured_payload_size;
    const int frame_size_bytes = frame_size_bits / 8;
    const int audio_sample_rate = 8000;     // 16-bit samples per second for audio signals
    const int audio_frame_size = audio_sample_rate * 0.02;  // samples per audio frame
    const int opus_frame_size_bytes = 40;   // bytes in an encoded 20ms Opus frame

    using plheader_t = std::array<int8_t, encoded_plheader_size>;   // one bit per int8_t

}