// Copyright 2021 Mobilinkd LLC.

#pragma once

#include "SlidingDFT.h"

#include <array>
#include <complex>
#include <cstddef>
#include <iostream>

namespace mobilinkd {

/**
 * Data carrier detection using the difference of two DFTs, one in-band and
 * one out-of-band.  The first frequency is the in-band frequency and the
 * second one is the out-of-band Frequency.  The second frequency must be
 * within the normal passband of the receiver, but beyond the normal roll-off
 * frequency of the data carrier.
 *
 * This version uses the NSlidingDFT implementation to reduce the memory
 * footprint.
 *
 * As an example, the cut-off for 4.8k symbol/sec 4-FSK is 2400Hz, so 3000Hz
 * is a reasonable out-of-band frequency to use.
 *
 * Note: the input to this DCD must be unfiltered (raw) baseband input.
 */
template <typename FloatType, size_t SampleRate, size_t Accuracy = 1000>
struct DataCarrierDetect
{
    using ComplexType = std::complex<FloatType>;
    using NDFT = NSlidingDFT<FloatType, SampleRate, SampleRate / Accuracy, 2>;

    NDFT dft_;
    FloatType trigger_;
    FloatType level_1 = 0.0;
    FloatType level_2 = 0.0;
    FloatType level_ = 0.0;

    DataCarrierDetect(
        size_t freq1, size_t freq2,
        FloatType trigger = 100.0
        )
    : dft_({freq1, freq2}), trigger_(trigger)
    {
    }

    /**
     * Accept unfiltered baseband input and output a decision on whether
     * a carrier has been detected after every @tparam BlockSize inputs.
     */
    void operator()(FloatType sample)
    {
        auto result = dft_(sample);
        level_1 += std::norm(result[0]);
        level_2 += std::norm(result[1]);
    }

    /**
     * Update the data carrier detection level.
     */
    void update()
    {
    	level_ = level_ * 0.8 + 0.2 * (level_1 / level_2);
    	level_1 = 0.0;
    	level_2 = 0.0;
    }


    FloatType level() const { return level_; }
    bool dcd() const { return level_ > trigger_; }
};

} // mobilinkd
