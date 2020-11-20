// Copyright 2020 Mobilinkd LLC.

#pragma once

#include <array>
#include <cstdint>

namespace mobilinkd
{

template <size_t N = 368>
struct M17Framer
{
    using buffer_t = std::array<int8_t, N>;

    alignas(16) buffer_t buffer_;
    size_t index_ = 0;

    M17Framer()
    {
        reset();
    }

    static constexpr size_t size() { return N; }

    size_t operator()(int bit, int8_t** result)
    {
        buffer_[index_++] = bit;
        if (index_ == N)
        {
            index_ = 0;
            *result = buffer_.begin();
            return N;
        }
        return 0;
    }
    
    void reset()
    { 
        buffer_.fill(0);
        index_ = 0;
    }
};

} // mobilinkd
