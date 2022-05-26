// Copyright 2022 Mobilinkd LLC.

#pragma once

#include "KalmanFilter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <cassert>

namespace mobilinkd
{

template <typename FloatType, size_t SamplesPerSymbol>
struct ClockRecovery
{
    m17::KalmanFilter<FloatType, SamplesPerSymbol> kf_;
    size_t count_ = 0;
    int8_t sample_index_ = 0;
    FloatType clock_estimate_ = 0.;
    FloatType sample_estimate_ = 0.;

    /**
     * Reset the clock recovery after the first sync word is received.
     * This is used as the starting state of the Kalman filter. Providing
     * the filter with a realistic starting point causes it to converge
     * must faster.
     * 
     * @param[in] index starting sample index.
     */
    void reset(FloatType index)
    {
        kf_.reset(index);
        count_ = 0;
        sample_index_ = index;
        clock_estimate_ = 0.;
    }

    /// Count each sample.
    void operator()(FloatType)
    {
        ++count_;
    }

    /**
     * Update the filter with the estimated index from the sync word. The
     * result is a new estimate of the state, the remote symbol clock offset
     * relative to our clock.  It can be faster (>0.0) or slower (<0.0).
     * 
     * @param[in] index is the new symbol sample position from the sync word.
     */
    bool update(uint8_t index)
    {
        auto f = kf_.update(index, count_);

        // Constrain sample index to [0..SamplesPerSymbol), wrapping if needed.
        sample_estimate_ = f[0];
        sample_index_ = int8_t(round(sample_estimate_));
        sample_index_ = sample_index_ < 0 ? sample_index_ + SamplesPerSymbol : sample_index_;
        sample_index_ = sample_index_ >= int8_t(SamplesPerSymbol) ? sample_index_ - SamplesPerSymbol : sample_index_;
        clock_estimate_ = f[1];
        count_ = 0;

        return true;
    }

    /**
     * This is used when no sync word is found. The sample index is updated
     * based on the current clock estimate, the last known good sample
     * estimate, and the number of samples processed.
     *
     * The sample and clock estimates from the filter remain unchanged.
     */
    bool update()
    {
        auto csw = std::fmod((sample_estimate_ + clock_estimate_ * count_), SamplesPerSymbol);
        if (csw < 0.) csw += SamplesPerSymbol;
        else if (csw >= SamplesPerSymbol) csw -= SamplesPerSymbol;

        // Constrain sample index to [0..SamplesPerSymbol), wrapping if needed.
        sample_index_ = int8_t(round(csw));
        sample_index_ = sample_index_ < 0 ? sample_index_ + SamplesPerSymbol : sample_index_;
        sample_index_ = sample_index_ >= int8_t(SamplesPerSymbol) ? sample_index_ - SamplesPerSymbol : sample_index_;

        return true;
    }

    /**
     * Return the estimated sample clock increment based on the last update.
     *
     * The value is only valid after samples have been collected and update()
     * has been called.
     */
    FloatType clock_estimate() const
    {
        return clock_estimate_;
    }

    /**
     * Return the estimated "best sample index" based on the last update.
     *
     * The value is only valid after samples have been collected and update()
     * has been called.
     */
    uint8_t sample_index() const
    {
        return sample_index_;
    }
};


} // mobilinkd
