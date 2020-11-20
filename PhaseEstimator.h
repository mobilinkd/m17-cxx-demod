// Copyright 2020 Mobilinkd LLC.

#pragma once

#include <array>
#include <algorithm>
#include <cassert>

namespace mobilinkd
{

/**
 * Estimate the phase of a sample by estimating the
 * tangent of the sample point.  This is done by computing
 * the magnitude difference of the previous and following
 * samples.  We do not correct for 0-crossing errors because
 * these errors have not affected the performance of clock
 * recovery.
 */
template <typename T>
struct PhaseEstimator
{
    using float_type = T;
    using samples_t = std::array<float_type, 3>;    // 3 samples in length
    using minmax_t = std::array<float_type, 32>;    // 32 symbols in length

    float_type dx_;
    minmax_t symbols_{0};
    size_t index_ = 0;

    PhaseEstimator(float_type sample_rate, float_type symbol_rate)
    : dx_(2.0 * symbol_rate / sample_rate)
    {
        symbols_.fill(0.0);
    }

    /**
     * This performs a rolling estimate of the phase.
     * 
     * @param samples are three samples centered around the current sample point
     *  (t-1, t, t+1).
     */
    float_type operator()(const samples_t& samples)
    {
        assert(dx_ > 0.0);

        symbols_[index_++] = samples[1];

        if (index_ == std::tuple_size<minmax_t>::value) index_ = 0;

        auto symbol_min = *std::min_element(std::begin(symbols_), std::end(symbols_));
        auto symbol_max = *std::max_element(std::begin(symbols_), std::end(symbols_));

        auto y_scale = (symbol_max - symbol_min) * 0.5;
        auto dy = y_scale != 0 ? (samples.at(2) - samples.at(0)) / y_scale : 0;
        
        auto ratio = dy / dx_;
        // Clamp +/-5.
        ratio = std::min(float_type(5.0), ratio);
        ratio = std::max(float_type(-5.0), ratio);
        
        return ratio;
    }
};

} // mobilinkd
