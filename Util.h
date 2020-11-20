// Copyright 2020 Mobilinkd LLC.

#pragma once

#include <cstdlib>

namespace mobilinkd
{

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

} // mobilinkd
