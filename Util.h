// Copyright 2020 Mobilinkd LLC.

#pragma once

#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <array>
#include <bitset>
#include <tuple>

namespace mobilinkd
{

// The make_bitset stuff only works as expected in GCC10 and later.

namespace detail {

template<std::size_t...Is, class Tuple>
constexpr std::bitset<sizeof...(Is)> make_bitset(std::index_sequence<Is...>, Tuple&& tuple)
{
    constexpr auto size = sizeof...(Is);
    std::bitset<size> result;
    using expand = int[];
    for (size_t i = 0; i != size; ++i)
    {
        void(expand {0, result[Is] = std::get<Is>(tuple)...});
    }
    return result;
}

}

template<class...Bools>
constexpr auto make_bitset(Bools&&...bools)
{
    return detail::make_bitset(std::make_index_sequence<sizeof...(Bools)>(),
        std::make_tuple(bool(bools)...));
}

inline int from_4fsk(int symbol)
{
    // Convert a 4-FSK symbol to a pair of bits.
    switch (symbol)
    {
        case 1: return 0;
        case 3: return 1;
        case -1: return 2;
        case -3: return 3;
        default: abort();
    }
}

#if 0
inline std::pair<int8_t, int8_t> 4fsk_demod(int symbol)
{
    // Convert a 4-FSK symbol to a pair of bits.
    switch (symbol)
    {
        case 1: return std::pair<int8_t, int8_t>(0,0);
        case 3: return std::pair<int8_t, int8_t>(0,1);
        case -1: return std::pair<int8_t, int8_t>(1,0);
        case -3: return std::pair<int8_t, int8_t>(1,1);
        default: abort();
    }
}
#endif 

template <size_t M, typename T, size_t N, typename U, size_t IN>
auto depunctured(std::array<T, N> puncture_matrix, std::array<U, IN> in)
{
    static_assert(M % N == 0);
    std::array<U, M> result;
    size_t index = 0;
    size_t pindex = 0;
    for (size_t i = 0; i != M; ++i)
    {
        if (!puncture_matrix[pindex++])
        {
            result[i] = 0;
        }
        else
        {
            result[i] = in[index++];
        }
        if (pindex == N) pindex = 0;
    }
    return result;
}

template <size_t IN, size_t OUT, size_t P>
size_t depuncture(const std::array<int8_t, IN>& in,
    std::array<int8_t, OUT>& out, const std::array<int8_t, P>& p)
{
    size_t index = 0;
    size_t pindex = 0;
    size_t bit_count = 0;
    for (size_t i = 0; i != OUT && index < IN; ++i)
    {
        if (!p[pindex++])
        {
            out[i] = 0;
            bit_count++;
        }
        else
        {
            out[i] = in[index++];
        }
        if (pindex == P) pindex = 0;
    }
    return bit_count;
}

/**
 * Sign-extend an n-bit value to a specific signed integer type.
 */
template <typename T, size_t n>
constexpr T to_int(uint8_t v)
{
    constexpr auto MAX_INPUT = (1 << (n - 1));
    constexpr auto NEGATIVE_OFFSET = std::numeric_limits<typename std::make_unsigned<T>::type>::max() - (MAX_INPUT - 1);
    T r = v & (1 << (n - 1)) ? NEGATIVE_OFFSET : 0;
    return r + (v & (MAX_INPUT - 1));
}

template <typename T, size_t N>
constexpr auto to_byte_array(std::array<T, N> in)
{
    std::array<uint8_t, (N + 7) / 8> out{};
    out.fill(0);
    size_t i = 0;
    size_t b = 0;
    for (auto c : in)
    {
        out[i] |= (c << (7 - b));
        if (++b == 8)
        {
            ++i;
            b = 0;
        }
    }
    return out;
}

} // mobilinkd
