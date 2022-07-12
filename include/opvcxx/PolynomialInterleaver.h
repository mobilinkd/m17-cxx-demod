// Copyright 2020 Mobilinkd LLC.
// Copyright 2022 Open Research Institute, Inc.

#pragma once

#include "Util.h"
#include "Numerology.h"

#include <algorithm>
#include <array>

namespace mobilinkd
{

// This interleaver is optimized for 16,000bps Opulent Voice frames,
// and achieves a minimum distance proportional to that of the M17
// interleaver. 
template <size_t F1= PolynomialInterleaverX, size_t F2 = PolynomialInterleaverX2, size_t K = stream_type4_size>
struct PolynomialInterleaver
{
    using buffer_t = std::array<int8_t, K>;
    using bytes_t = std::array<uint8_t, K / 8>;

    alignas(16) buffer_t buffer_;

    size_t index(size_t i)
    {
        return ((F1 * i) + (F2 * i * i)) % K;
    }
    
    void interleave(buffer_t& data)
    {
        buffer_.fill(0);

        for (size_t i = 0; i != K; ++i)
            buffer_[index(i)] = data[i];
        
        std::copy(std::begin(buffer_), std::end(buffer_), std::begin(data));
    }

    void interleave(bytes_t& data)
    {
        bytes_t buffer;
        buffer.fill(0);
        for (size_t i = 0; i != K; ++i)
        {
            assign_bit_index(buffer, index(i), get_bit_index(data, i));
        }
        std::copy(buffer.begin(), buffer.end(), data.begin());
    }

    void deinterleave(buffer_t& frame)
    {
        buffer_.fill(0);

        for (size_t i = 0; i != K; ++i)
        {
            auto idx = index(i);
            buffer_[i] = frame[idx];
        }
        
        std::copy(buffer_.begin(), buffer_.end(), frame.begin());
    }

    void deinterleave(bytes_t& data)
    {
        bytes_t buffer;
        buffer.fill(0);
        for (size_t i = 0; i != K; ++i)
        {
            assign_bit_index(buffer, i, get_bit_index(data, index(i)));
        }
        std::copy(buffer.begin(), buffer.end(), data.begin());
    }

};

} // mobilinkd
