// Copyright 2021 Rob Riggs <rob@mobilinkd.com>
// All rights reserved.

#pragma once

#include "KalmanFilter.h"

#include <cmath>
#include <cstddef>

namespace mobilinkd {

template <typename FloatType>
class FreqDevEstimator
{
    static constexpr FloatType DEVIATION = 2400.;

    m17::SymbolKalmanFilter<FloatType> minFilter_;
    m17::SymbolKalmanFilter<FloatType> maxFilter_;
    FloatType idev_ = 0.;
    FloatType offset_ = 0.;
    bool reset_ = true;

public:

    void reset()
    {
        reset_ = true;
    }

    void update(FloatType minValue, FloatType maxValue)
    {
        auto mnf = minFilter_.update(minValue, 192);
        auto mxf = maxFilter_.update(maxValue, 192);
        offset_ = mxf[0] + mnf[0];
        idev_ = 6.0 / (mxf[0] - mnf[0]);

        if (isnan(mnf) || isnan(mxf)) reset_ = true;

        if (reset_)
        {
            reset_ = false;
            minFilter_.reset(minValue);
            maxFilter_.reset(maxValue);
            offset_ = minValue + maxValue;
            idev_ = 6.0 / (maxValue - minValue);
        }
    }

    FloatType idev() const { return idev_; }
    FloatType offset() const { return offset_; }
    FloatType deviation() const { return DEVIATION / idev_; }
    FloatType error() const { return 0.; }
};

} // mobilinkd
