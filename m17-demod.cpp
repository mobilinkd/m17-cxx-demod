// Copyright 2020 Mobilinkd LLC.

#include "m17-demod.h"
#include "Fsk4Demod.h"
#include "Util.h"
#include "CarrierDetect.h"
#include "M17Synchronizer.h"
#include "M17Framer.h"
#include "M17FrameDecoder.h"

#include <array>
#include <experimental/array>
#include <iostream>
#include <iomanip>

#include <cstdlib>

// Generated using scikit-commpy
const auto rrc_taps = std::experimental::make_array<double>(
    -0.009265784007800534, -0.006136551625729697, -0.001125978562075172, 0.004891777252042491,
    0.01071805138282269, 0.01505751553351295, 0.01679337935001369, 0.015256245142156299,
    0.01042830577908502, 0.003031522725559901,  -0.0055333532968188165, -0.013403099825723372,
    -0.018598682349642525, -0.01944761739590459, -0.015005271935951746, -0.0053887880354343935,
    0.008056525910253532, 0.022816244158307273, 0.035513467692208076, 0.04244131815783876,
    0.04025481153629372, 0.02671818654865632, 0.0013810216516704976, -0.03394615682795165,
    -0.07502635967975885, -0.11540977897637611, -0.14703962203941534, -0.16119995609538576,
    -0.14969512896336504, -0.10610329539459686, -0.026921412469634916, 0.08757875030779196,
    0.23293327870303457, 0.4006012210123992, 0.5786324696325503, 0.7528286479934068,
    0.908262741447522, 1.0309661131633199, 1.1095611856548013, 1.1366197723675815,
    1.1095611856548013, 1.0309661131633199, 0.908262741447522, 0.7528286479934068,
    0.5786324696325503,  0.4006012210123992, 0.23293327870303457, 0.08757875030779196,
    -0.026921412469634916, -0.10610329539459686, -0.14969512896336504, -0.16119995609538576,
    -0.14703962203941534, -0.11540977897637611, -0.07502635967975885, -0.03394615682795165,
    0.0013810216516704976, 0.02671818654865632, 0.04025481153629372,  0.04244131815783876,
    0.035513467692208076, 0.022816244158307273, 0.008056525910253532, -0.0053887880354343935,
    -0.015005271935951746, -0.01944761739590459, -0.018598682349642525, -0.013403099825723372,
    -0.0055333532968188165, 0.003031522725559901, 0.01042830577908502, 0.015256245142156299,
    0.01679337935001369, 0.01505751553351295, 0.01071805138282269, 0.004891777252042491,
    -0.001125978562075172, -0.006136551625729697, -0.009265784007800534
);

const auto evm_b = std::experimental::make_array<double>(0.02008337, 0.04016673, 0.02008337);
const auto evm_a = std::experimental::make_array<double>(1.0, -1.56101808, 0.64135154);

int main(int argc, char* argv[])
{
    using namespace mobilinkd;

    auto demod = Fsk4Demod(48000.0, 4800.0, 0.03);
    auto dcd = CarrierDetect<double, 10>(0.1, 0.4);
    auto synch = M17Synchronizer();
    auto framer = M17Framer();
    auto decoder = M17FrameDecoder();

    int count = 0;
    enum class State { UNLOCKED, SYNCHING, FRAMING};
    State state = State::UNLOCKED;
    int8_t* frame;
    alignas(16) std::array<int8_t, framer.size()> buffer;
    uint32_t ber = 0;

    while (std::cin)
    {
        int16_t sample;
        std::cin.read(reinterpret_cast<char*>(&sample), 2);
        auto result = demod(sample / 5000.0);
        if (result)
        {
            count += 1;
            auto [prev_sample, phase_estimate, symbol, evm, estimated_deviation, estimated_frequency_offset] = *result;
            auto [locked, rms] = dcd(estimated_frequency_offset);

            switch (state)
            {
            case State::UNLOCKED:
                if (locked)
                {
                    state = State::SYNCHING;
                    // std::cout << "Lock!" << std::endl;
                    decoder.reset();
                    ber = -1;
                }
                break;
            case State::SYNCHING:
                if (!locked)
                {
                    state = State::UNLOCKED;
                    // std::cout << "Unlock!" << std::endl;
                }
                else if (synch(from_4fsk(symbol)))
                {
                    state = State::FRAMING;
                    // std::cout << "Sync!" << std::endl;
                }
                break;
            case State::FRAMING:
                {
                    auto n = from_4fsk(symbol);
                    auto len = framer(n >> 1, &frame);
                    assert(len == 0); // frames must end on symbol boundaries.
                    len = framer(n & 1, &frame);
                    if (len != 0)
                    {
                        state = State::SYNCHING;
                        std::copy(frame, frame + len, buffer.begin());
                        // std::cout << "Frame!" << std::endl;
                        ber = decoder(buffer);
                    }
                }
                break;
            }

            if ((count % 192) == 0)
            {
                std::cerr << "\r count: " << std::setw(8) << count
                    << ", phase: " << std::setprecision(2) << std::setw(8) << phase_estimate
                    << ", evm: " << std::setprecision(2) << std::setw(8) << rms
                    << ", deviation: " << std::setprecision(2) << std::setw(8) << estimated_deviation
                    << ", freq offset: " << std::setprecision(2) << std::setw(8) << estimated_frequency_offset
                    << ", locked: " << std::boolalpha << std::setw(6) << locked
                    << ", ber: " << ber << std::ends;
            }
        }
    }

    std::cerr << std::endl;
    // std::cout << std::endl;

    return EXIT_SUCCESS;
}
