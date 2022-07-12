#pragma once

#include <array>
#include <cassert>

namespace mobilinkd
{
    const int opus_bitrate = 16000;         // target output bit rate from voice encoder

    const int audio_sample_rate = 48000;     // 16-bit PCM samples per second for audio signals
    const int audio_frame_size = audio_sample_rate * 0.04;  // PCM samples per audio frame (two codec frames)
    const int audio_frame_sample_bytes = audio_frame_size * 2;  // 2 bytes per PCM sample

    // Simplified Frame Format for OPUlent Voice
    const int fheader_size_bytes = 12;        // bytes in a frame header (multiple of 3 for Golay encoding)
    const int encoded_fheader_size = fheader_size_bytes * 8 * 2;    // bits in an encoded frame header

    const int opus_frame_size_bytes = 40;   // bytes in an encoded 20ms Opus frame
    const int stream_frame_payload_bytes = 2 * opus_frame_size_bytes;  // All frames carry this much type1 payload data
    const int stream_frame_payload_size = 8 * stream_frame_payload_bytes;
    const int stream_frame_type1_size = stream_frame_payload_size + 4;  // add encoder tail bits
    const int stream_type2_payload_size = 2 * stream_frame_type1_size;  // Encoding type1 to type2 doubles the size, plus encoder tail
    const int stream_type3_payload_size = stream_type2_payload_size;    // no puncturing in OPUlent Voice
    const int stream_type4_size = encoded_fheader_size + stream_type3_payload_size;
    const int stream_type4_bytes = stream_type4_size / 8;
    
    const int baseband_frame_symbols = 16 / 2 + stream_type4_size / 2;   // dibits or symbols in sync+payload in a frame

    const int bert_frame_total_size = stream_frame_payload_size;
    const int bert_frame_prime_size = 631;      // largest prime smaller than bert_frame_total_size

    const int symbol_rate = baseband_frame_symbols / 0.04;  // symbols per second
    const int sample_rate = symbol_rate * 10;               // sample rate
    const int samples_per_frame = sample_rate * 0.04;       // samples per 40ms frame

    static_assert((stream_type3_payload_size % 8) == 0, "Type3 payload size not an integer number of bytes");
    static_assert(bert_frame_prime_size < bert_frame_total_size, "BERT prime size not less than BERT total size");
    static_assert(bert_frame_prime_size % 2 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 3 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 5 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 7 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 11 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 13 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 17 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 19 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 23 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 29 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 31 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 37 != 0, "BERT prime size not prime");
    static_assert(bert_frame_prime_size % 41 != 0, "BERT prime size not prime");
}