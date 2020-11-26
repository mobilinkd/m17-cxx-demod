// Copyright 2020 Mobilinkd LLC.

#pragma once

#include <array>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace mobilinkd
{

template <typename FloatType, size_t N = 10>
struct CarrierDetect
{
    using result_t = std::tuple<bool, FloatType>;

    FloatType lock_;
    FloatType unlock_;
    std::array<FloatType, N> samples_;
    std::array<FloatType, N> variances_;
    FloatType sum_ = 0.0;
    FloatType var_ = 0.0;
    size_t index_ = 0;
    bool locked_ = false;

    CarrierDetect(FloatType lock_level, FloatType unlock_level)
    : lock_(lock_level), unlock_(unlock_level)
    {
        samples_.fill(0.0);
    }
    
    result_t operator()(FloatType evm)
    {
        sum_ = sum_ - samples_[index_] + evm;
        auto var = evm - (sum_ / N);
        var_ = var_ - variances_[index_] + (var * var);
        variances_[index_] = (var * var);
        samples_[index_++] = evm;
        if (index_ == N) index_ = 0;
        
        auto variance = var_ / N;
        auto stdev = sqrt(variance);
        if (!locked_)
        {
            if (stdev < lock_) locked_ = true;
        }
        else
        {
            if (stdev > unlock_) locked_ = false;
        }

        return std::make_tuple(locked_, stdev);
    }
};

} // mobilinkd
