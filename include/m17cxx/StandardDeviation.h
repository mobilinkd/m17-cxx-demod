// Copyright 2020-2022 Mobilinkd LLC <rob@mobilinkd.com>
// All rights reserved.

#pragma once

#include <cmath>
#include <cstdint>

namespace mobilinkd {

/**
 * Compute a running standard deviation. Avoids having to store
 * all of the samples.
 *
 * Based on https://dsp.stackexchange.com/a/1187/36581
 */
template <typename FloatType>
struct StandardDeviation
{
    FloatType mean{0.0};
    FloatType S{0.0};
    size_t samples{0};

    void reset()
    {
        mean = 0.0;
        S = 0.0;
        samples = 0;
    }

    void capture(float sample)
    {
        auto prev = mean;
        samples += 1;
        mean = mean + (sample - mean) / samples;
        S = S + (sample - mean) * (sample - prev);
    }

    FloatType variance() const
    {
        return samples == 0 ? -1.0 :  S / samples;
    }

    FloatType stdev() const
    {
        FloatType result = -1.0;
        if (samples) result = std::sqrt(variance());
        return result;
    }

    // SNR in dB
    FloatType SNR() const
    {
        return 10.0 * std::log10(mean / stdev());
    }
};
template <typename FloatType, size_t N>
struct RunningStandardDeviation
{
    FloatType S{1.0};
    FloatType alpha{1.0 / N};

    void reset()
    {
        S = 0.0;
    }

    void capture(float sample)
    {
        S -= S * alpha;
        S += (sample * sample) * alpha;
    }

    FloatType variance() const
    {
        return S;
    }

    FloatType stdev() const
    {
        return std::sqrt(variance());
    }
};

} // mobilinkd
