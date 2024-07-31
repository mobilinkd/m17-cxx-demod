// Copyright 2020-2022 Mobilinkd LLC.

#pragma once

#include "StandardDeviation.h"

namespace mobilinkd
{

/**
 * Compute the EVM of a symbol. This assumes that the incoming sample has
 * been normalized, meaning its offset and scale has been adjusted so that
 * nominal values fall exactly on the symbol values of -3, -1, 1, 3. It
 * determines the nearest symbol value and computes the variance.
 * 
 * This uses a running standard deviation with a nominal length of 184
 * symbols. That is the payload size of an M17 frame.
 */
template <typename FloatType>
struct SymbolEvm
{
    RunningStandardDeviation<FloatType, 184> stddev;
   
    void reset()
    {
        stddev.reset();
    }

    FloatType evm() const { return stddev.stdev(); }

    void update(FloatType sample)
    {
        FloatType evm;

        if (sample > 2)
        {
            stddev.capture(sample - 3);
        }
        else if (sample > 0)
        {
            stddev.capture(sample - 1);
        }
        else if (sample > -2)
        {
            stddev.capture(sample + 1);
        }
        else
        {
            stddev.capture(sample + 3);
        }
    }
};

} // mobilinkd
