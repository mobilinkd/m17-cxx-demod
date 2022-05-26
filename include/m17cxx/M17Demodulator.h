// Copyright 2020-2021 Rob Riggs <rob@mobilinkd.com>
// All rights reserved.

#pragma once

#include "ClockRecovery.h"
#include "Correlator.h"
#include "DataCarrierDetect.h"
#include "FirFilter.h"
#include "FreqDevEstimator.h"
#include "M17FrameDecoder.h"
#include "M17Framer.h"
#include "SymbolEvm.h"
#include "Util.h"

#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <tuple>

namespace mobilinkd {

namespace detail
{

template <typename FloatType>
struct Taps
{};

template <>
struct Taps<double>
{
    static constexpr auto rrc_taps = std::experimental::make_array<double>(
        0.0029364388513841593, 0.0031468394550958484, 0.002699564567597445, 0.001661182944400927,
        0.00023319405581230247, -0.0012851320781224025, -0.0025577136087664687, -0.0032843366522956313,
        -0.0032697038088887226, -0.0024733964729590865, -0.0010285696910973807, 0.0007766690889758685,
        0.002553421969211845, 0.0038920145144327816, 0.004451886520053017, 0.00404219185231544,
        0.002674727068399207, 0.0005756567993179152, -0.0018493784971116507, -0.004092346891623224,
        -0.005648131453822014, -0.006126925416243605, -0.005349511529163396, -0.003403189203405097,
        -0.0006430502751187517, 0.002365929161655135, 0.004957956568090113, 0.006506845894531803,
        0.006569574194782443, 0.0050017573119839134, 0.002017321931508163, -0.0018256054303579805,
        -0.00571615173291049, -0.008746639552588416, -0.010105075751866371, -0.009265784007800534,
        -0.006136551625729697, -0.001125978562075172, 0.004891777252042491, 0.01071805138282269,
        0.01505751553351295, 0.01679337935001369, 0.015256245142156299, 0.01042830577908502,
        0.003031522725559901, -0.0055333532968188165, -0.013403099825723372, -0.018598682349642525,
        -0.01944761739590459, -0.015005271935951746, -0.0053887880354343935, 0.008056525910253532,
        0.022816244158307273, 0.035513467692208076, 0.04244131815783876, 0.04025481153629372,
        0.02671818654865632, 0.0013810216516704976, -0.03394615682795165, -0.07502635967975885,
        -0.11540977897637611, -0.14703962203941534, -0.16119995609538576, -0.14969512896336504,
        -0.10610329539459686, -0.026921412469634916, 0.08757875030779196, 0.23293327870303457,
        0.4006012210123992, 0.5786324696325503, 0.7528286479934068, 0.908262741447522,
        1.0309661131633199, 1.1095611856548013, 1.1366197723675815, 1.1095611856548013,
        1.0309661131633199, 0.908262741447522, 0.7528286479934068, 0.5786324696325503,
        0.4006012210123992, 0.23293327870303457, 0.08757875030779196, -0.026921412469634916,
        -0.10610329539459686, -0.14969512896336504, -0.16119995609538576, -0.14703962203941534,
        -0.11540977897637611, -0.07502635967975885, -0.03394615682795165, 0.0013810216516704976,
        0.02671818654865632, 0.04025481153629372, 0.04244131815783876, 0.035513467692208076,
        0.022816244158307273, 0.008056525910253532, -0.0053887880354343935, -0.015005271935951746,
        -0.01944761739590459, -0.018598682349642525, -0.013403099825723372, -0.0055333532968188165,
        0.003031522725559901, 0.01042830577908502, 0.015256245142156299, 0.01679337935001369,
        0.01505751553351295, 0.01071805138282269, 0.004891777252042491, -0.001125978562075172,
        -0.006136551625729697, -0.009265784007800534, -0.010105075751866371, -0.008746639552588416,
        -0.00571615173291049, -0.0018256054303579805, 0.002017321931508163, 0.0050017573119839134,
        0.006569574194782443, 0.006506845894531803, 0.004957956568090113, 0.002365929161655135,
        -0.0006430502751187517, -0.003403189203405097, -0.005349511529163396, -0.006126925416243605,
        -0.005648131453822014, -0.004092346891623224, -0.0018493784971116507, 0.0005756567993179152,
        0.002674727068399207, 0.00404219185231544, 0.004451886520053017, 0.0038920145144327816,
        0.002553421969211845, 0.0007766690889758685, -0.0010285696910973807, -0.0024733964729590865,
        -0.0032697038088887226, -0.0032843366522956313, -0.0025577136087664687, -0.0012851320781224025,
        0.00023319405581230247, 0.001661182944400927, 0.002699564567597445, 0.0031468394550958484,
        0.0029364388513841593, 0.0
    );
};

template <>
struct Taps<float>
{
    static constexpr auto rrc_taps = std::experimental::make_array<float>(
        0.0029364388513841593, 0.0031468394550958484, 0.002699564567597445, 0.001661182944400927,
        0.00023319405581230247, -0.0012851320781224025, -0.0025577136087664687, -0.0032843366522956313,
        -0.0032697038088887226, -0.0024733964729590865, -0.0010285696910973807, 0.0007766690889758685,
        0.002553421969211845, 0.0038920145144327816, 0.004451886520053017, 0.00404219185231544,
        0.002674727068399207, 0.0005756567993179152, -0.0018493784971116507, -0.004092346891623224,
        -0.005648131453822014, -0.006126925416243605, -0.005349511529163396, -0.003403189203405097,
        -0.0006430502751187517, 0.002365929161655135, 0.004957956568090113, 0.006506845894531803,
        0.006569574194782443, 0.0050017573119839134, 0.002017321931508163, -0.0018256054303579805,
        -0.00571615173291049, -0.008746639552588416, -0.010105075751866371, -0.009265784007800534,
        -0.006136551625729697, -0.001125978562075172, 0.004891777252042491, 0.01071805138282269,
        0.01505751553351295, 0.01679337935001369, 0.015256245142156299, 0.01042830577908502,
        0.003031522725559901, -0.0055333532968188165, -0.013403099825723372, -0.018598682349642525,
        -0.01944761739590459, -0.015005271935951746, -0.0053887880354343935, 0.008056525910253532,
        0.022816244158307273, 0.035513467692208076, 0.04244131815783876, 0.04025481153629372,
        0.02671818654865632, 0.0013810216516704976, -0.03394615682795165, -0.07502635967975885,
        -0.11540977897637611, -0.14703962203941534, -0.16119995609538576, -0.14969512896336504,
        -0.10610329539459686, -0.026921412469634916, 0.08757875030779196, 0.23293327870303457,
        0.4006012210123992, 0.5786324696325503, 0.7528286479934068, 0.908262741447522,
        1.0309661131633199, 1.1095611856548013, 1.1366197723675815, 1.1095611856548013,
        1.0309661131633199, 0.908262741447522, 0.7528286479934068, 0.5786324696325503,
        0.4006012210123992, 0.23293327870303457, 0.08757875030779196, -0.026921412469634916,
        -0.10610329539459686, -0.14969512896336504, -0.16119995609538576, -0.14703962203941534,
        -0.11540977897637611, -0.07502635967975885, -0.03394615682795165, 0.0013810216516704976,
        0.02671818654865632, 0.04025481153629372, 0.04244131815783876, 0.035513467692208076,
        0.022816244158307273, 0.008056525910253532, -0.0053887880354343935, -0.015005271935951746,
        -0.01944761739590459, -0.018598682349642525, -0.013403099825723372, -0.0055333532968188165,
        0.003031522725559901, 0.01042830577908502, 0.015256245142156299, 0.01679337935001369,
        0.01505751553351295, 0.01071805138282269, 0.004891777252042491, -0.001125978562075172,
        -0.006136551625729697, -0.009265784007800534, -0.010105075751866371, -0.008746639552588416,
        -0.00571615173291049, -0.0018256054303579805, 0.002017321931508163, 0.0050017573119839134,
        0.006569574194782443, 0.006506845894531803, 0.004957956568090113, 0.002365929161655135,
        -0.0006430502751187517, -0.003403189203405097, -0.005349511529163396, -0.006126925416243605,
        -0.005648131453822014, -0.004092346891623224, -0.0018493784971116507, 0.0005756567993179152,
        0.002674727068399207, 0.00404219185231544, 0.004451886520053017, 0.0038920145144327816,
        0.002553421969211845, 0.0007766690889758685, -0.0010285696910973807, -0.0024733964729590865,
        -0.0032697038088887226, -0.0032843366522956313, -0.0025577136087664687, -0.0012851320781224025,
        0.00023319405581230247, 0.001661182944400927, 0.002699564567597445, 0.0031468394550958484,
        0.0029364388513841593, 0.0
    );
};

} // detail

template <typename FloatType>
struct M17Demodulator
{
    static constexpr uint16_t SAMPLE_RATE = 48000;
    static constexpr uint16_t SYMBOL_RATE = 4800;
    static constexpr uint16_t SAMPLES_PER_SYMBOL = SAMPLE_RATE / SYMBOL_RATE;
    static constexpr uint16_t BLOCK_SIZE = 192;

    static constexpr FloatType sample_rate = SAMPLE_RATE;
    static constexpr FloatType symbol_rate = SYMBOL_RATE;

    static constexpr size_t STREAM_COST_LIMIT = 80;
    static constexpr size_t PACKET_COST_LIMIT = 60;
    static constexpr uint8_t MAX_MISSING_SYNC = 10;
    static constexpr uint8_t MIN_SYNC_COUNT = 78;
    static constexpr uint8_t MAX_SYNC_COUNT = 86;
    static constexpr FloatType EOT_TRIGGER_LEVEL = 0.1;

    using collelator_t = Correlator<FloatType>;
    using sync_word_t = SyncWord<collelator_t>;
    using callback_t = M17FrameDecoder::callback_t;
    using diagnostic_callback_t = std::function<void(bool, FloatType, FloatType, FloatType, bool, FloatType, int, int, int, int)>;

    enum class DemodState { UNLOCKED, LSF_SYNC, STREAM_SYNC, PACKET_SYNC, BERT_SYNC, SYNC_WAIT, FRAME };

    BaseFirFilter<FloatType, detail::Taps<FloatType>::rrc_taps.size()> demod_filter{detail::Taps<FloatType>::rrc_taps};
    DataCarrierDetect<FloatType, SAMPLE_RATE, 400> dcd{2400, 3600, 0.1, 4.0};
    ClockRecovery<FloatType, SAMPLES_PER_SYMBOL> clock_recovery;
    SymbolEvm<FloatType> evm;

    collelator_t correlator;
    sync_word_t preamble_sync{{+3,-3,+3,-3,+3,-3,+3,-3}, 29.f};
    sync_word_t lsf_sync{{+3,+3,+3,+3,-3,-3,+3,-3}, 31.f, -31.f};    // LSF or STREAM (inverted)
    sync_word_t packet_sync{{3,-3,3,3,-3,-3,-3,-3}, 31.f, -31.f};    // PACKET or BERT (inverted)
    sync_word_t eot_sync{{+3,+3,+3,+3,+3,+3,-3,+3}, 31.f};

    FreqDevEstimator<FloatType> dev;
    size_t count_ = 0;

    int8_t polarity = 1;
    M17Framer<368> framer;
    M17FrameDecoder decoder;
    DemodState demodState = DemodState::UNLOCKED;
    M17FrameDecoder::SyncWordType sync_word_type = M17FrameDecoder::SyncWordType::LSF;
    uint8_t sample_index = 0;

    bool dcd_ = false;
    bool need_clock_reset_ = false;
    bool need_clock_update_ = false;

    bool passall_ = false;
    size_t viterbi_cost = 0;
    int sync_count = 0;
    int missing_sync_count = 0;
    uint8_t sync_sample_index = 0;
    diagnostic_callback_t diagnostic_callback;

    M17Demodulator(callback_t callback)
    : decoder(callback)
    {}

    virtual ~M17Demodulator() {}

    void dcd_on();
    void dcd_off();
    void initialize(const FloatType input);
    void update_dcd();
    void do_unlocked();
    void do_lsf_sync();
    void do_packet_sync();
    void do_stream_sync();
    void do_bert_sync();
    void do_sync_wait();
    void do_frame(FloatType filtered_sample);

    bool locked() const
    {
        return dcd_;
    }

    void passall(bool enabled)
    {
    passall_ = enabled;
        // decoder.passall(enabled);
    }

    void diagnostics(diagnostic_callback_t callback)
    {
        diagnostic_callback = callback;
    }

    void update_values(uint8_t index);

    void operator()(const FloatType input);
};

template <typename FloatType>
void M17Demodulator<FloatType>::update_values(uint8_t index)
{
    sync_sample_index = index;
    auto [mn, mx] = correlator.outer_symbol_levels(sample_index);
    dev.update(mn, mx);
}

template <typename FloatType>
void M17Demodulator<FloatType>::dcd_on()
{
    // Data carrier newly detected.
    dcd_ = true;
    if (demodState == DemodState::UNLOCKED)
    {
        sync_count = 0;
        missing_sync_count = 0;

        framer.reset();
        decoder.reset();
        evm.reset();
    }
}

template <typename FloatType>
void M17Demodulator<FloatType>::dcd_off()
{
    // Just lost data carrier.
    demodState = DemodState::UNLOCKED;
    dcd_ = false;
}

template <typename FloatType>
void M17Demodulator<FloatType>::initialize(const FloatType input)
{
    auto filtered_sample = demod_filter(input);
    correlator.sample(filtered_sample);
}

template <typename FloatType>
void M17Demodulator<FloatType>::update_dcd()
{
    if (!dcd_ && dcd.dcd())
    {
        dcd_on();
        need_clock_reset_ = true;
    }
    else if (dcd_ && !dcd.dcd())
    {
        dcd_off();
    }
}

template <typename FloatType>
void M17Demodulator<FloatType>::do_unlocked()
{
    // We expect to find the preamble immediately after DCD.
    if (missing_sync_count < 1920)
    {
        missing_sync_count += 1;
        auto sync_index = preamble_sync(correlator);
        auto sync_updated = preamble_sync.updated();
        if (sync_updated)
        {
            sync_count = 0;
            missing_sync_count = 0;
            need_clock_reset_ = true;
            dev.reset();
            sample_index = sync_index;
            update_values(sync_index);
            demodState = DemodState::LSF_SYNC;
        }
        return;
    }
    auto sync_index = lsf_sync(correlator);
    auto sync_updated = lsf_sync.updated();
    if (sync_updated)
    {
        sync_count = MAX_SYNC_COUNT;
        missing_sync_count = 0;
        need_clock_reset_ = true;
        dev.reset();
        sample_index = sync_index;
        update_values(sync_index);
        demodState = DemodState::FRAME;
        if (sync_updated < 0)
        {
            sync_word_type = M17FrameDecoder::SyncWordType::STREAM;
        }
        else
        {
            sync_word_type = M17FrameDecoder::SyncWordType::LSF;
        }
    }
    sync_index = packet_sync(correlator);
    sync_updated = packet_sync.updated();
    if (sync_updated < 0)
    {
        sync_count = MAX_SYNC_COUNT;
        missing_sync_count = 0;
        need_clock_reset_ = true;
        dev.reset();
        sample_index = sync_index;
        update_values(sync_index);
        demodState = DemodState::FRAME;
        sync_word_type = M17FrameDecoder::SyncWordType::BERT;
    }
}

/**
 * Check for LSF sync word.  We only enter the DemodState::LSF_SYNC state
 * if a preamble sync has been detected, which also means that sample_index
 * has been initialized to a sane value for the baseband.
 */
template <typename FloatType>
void M17Demodulator<FloatType>::do_lsf_sync()
{
    FloatType sync_triggered = 0.;
    FloatType bert_triggered = 0.;

    if (correlator.index() == sample_index)
    {
        sync_triggered = preamble_sync.triggered(correlator);
        if (sync_triggered > 0.1)
        {
            sync_count += 1;
            return;
        }
        sync_triggered = lsf_sync.triggered(correlator);
        bert_triggered = packet_sync.triggered(correlator);
        if (bert_triggered < 0)
        {
            missing_sync_count = 0;
            sync_count = MAX_SYNC_COUNT;
            need_clock_update_ = true;
            update_values(sample_index);
            demodState = DemodState::FRAME;
            sync_word_type = M17FrameDecoder::SyncWordType::BERT;
        }
        else if (std::abs(sync_triggered) > 0.1)
        {
            missing_sync_count = 0;
            sync_count = MAX_SYNC_COUNT;
            need_clock_update_ = true;
            update_values(sample_index);
            if (sync_triggered > 0)
            {
                demodState = DemodState::FRAME;
                sync_word_type = M17FrameDecoder::SyncWordType::LSF;
            }
            else
            {
                demodState = DemodState::FRAME;
                sync_word_type = M17FrameDecoder::SyncWordType::STREAM;
            }
        }
        else if (++missing_sync_count > 192)
        {
            if (sync_count >= 10) {
                // Long preamble. Update clock and continue waiting for LSF.
                missing_sync_count = 0;
                need_clock_update_ = true;
            } else {
                // No sync word. Recycle.
                sync_count = 0;
                demodState = DemodState::UNLOCKED;
                missing_sync_count = 0;
                dcd.unlock();
            }
        }
        else
        {
            update_values(sample_index);
        }
    }
}

/**
 * Check for a stream sync word (LSF sync word that is maximally negative).
 * We can enter DemodState::STREAM_SYNC from either a valid LSF decode for
 * an audio stream, or from a stream frame decode.
 *
 */
template <typename FloatType>
void M17Demodulator<FloatType>::do_stream_sync()
{
    static bool eot_flag = false;
    
    sync_count += 1;
    if (sync_count < MIN_SYNC_COUNT) {
        return;
    }

    if (eot_sync.triggered(correlator) > EOT_TRIGGER_LEVEL) {
        // Note the EOT flag but continue trying to decode. This is needed
        // to avoid false triggers. If it is a true EOT, the stream will
        // end the next time we try to capture a sync word.
        sync_word_type = M17FrameDecoder::SyncWordType::STREAM;
        demodState = DemodState::FRAME;
        eot_flag = true;
        missing_sync_count = 0;
        return;
    }

    uint8_t sync_index = lsf_sync(correlator);
    int8_t sync_updated = lsf_sync.updated();
    if (sync_updated < 0)
    {
        missing_sync_count = 0;
        update_values(sync_index);
        sync_word_type = M17FrameDecoder::SyncWordType::STREAM;
        demodState = DemodState::SYNC_WAIT;
        eot_flag = false;
    }
    else if (sync_count > MAX_SYNC_COUNT)
    {
        // update_values(sync_index);
        if (viterbi_cost < STREAM_COST_LIMIT)
        {
            // Sync word missed but we are still decoding a stream reasonably
            // well. Don't increment the missing sync count, but it must not
            // be 0 when a sync word is missed for clock recovery to work. 
            if (!missing_sync_count) missing_sync_count = 1;
            sync_word_type = M17FrameDecoder::SyncWordType::STREAM;
            demodState = DemodState::FRAME;
        }
        else if (eot_flag) {
            // EOT flag set, missing sync, and very high BER. Stream has ended.
            demodState = DemodState::UNLOCKED;
            dcd.unlock();
        }
        else if (missing_sync_count < MAX_MISSING_SYNC)
        {
            // Sync word missed, very high error rate. Still trying to decode.
            missing_sync_count += 1;
            sync_word_type = M17FrameDecoder::SyncWordType::STREAM;
            demodState = DemodState::FRAME;
        }
        else
        {
            // No EOT, but too many sync words missed.  Recycle.
            demodState = DemodState::UNLOCKED;
            dcd.unlock();
        }
        eot_flag = false;
    }
}

/**
 * Check for a packet sync word.  DemodState::PACKET_SYNC can only be
 * entered from a valid LSF frame decode with the data/packet type bit set.
 */
template <typename FloatType>
void M17Demodulator<FloatType>::do_packet_sync()
{
    sync_count += 1;
    if (sync_count < MIN_SYNC_COUNT) {
        return;
    }

    auto sync_index = packet_sync(correlator);
    auto sync_updated = packet_sync.updated();

    if (sync_updated)
    {
        missing_sync_count = 0;
        update_values(sync_index);
        sync_word_type = M17FrameDecoder::SyncWordType::PACKET;
        demodState = DemodState::SYNC_WAIT;
    }
    else if (sync_count > MAX_SYNC_COUNT)
    {
        if (viterbi_cost < PACKET_COST_LIMIT)
        {
            // Sync word missed but we are still decoding reasonably well.
            if (!missing_sync_count) missing_sync_count = 1;
            sync_word_type = M17FrameDecoder::SyncWordType::PACKET;
            demodState = DemodState::FRAME;
        }
        else if (missing_sync_count < MAX_MISSING_SYNC)
        {
            // Sync word missed, very high error rate. Still trying to decode.
            // This may not be appropriate for packet mode.
            missing_sync_count += 1;
            sync_word_type = M17FrameDecoder::SyncWordType::PACKET;
            demodState = DemodState::FRAME;
        }
        else
        {
            // Too many sync words missed. Recycle.
            demodState = DemodState::UNLOCKED;
            dcd.unlock();
        }
    }
}

/**
 * Check for a bert sync word.
 */
template <typename FloatType>
void M17Demodulator<FloatType>::do_bert_sync()
{
    sync_count += 1;
    if (sync_count < MIN_SYNC_COUNT) {
        return;
    }

    auto sync_index = packet_sync(correlator);
    auto sync_updated = packet_sync.updated();

    if (sync_updated < 0)
    {
        missing_sync_count = 0;
        update_values(sync_index);
        sync_word_type = M17FrameDecoder::SyncWordType::BERT;
        demodState = DemodState::SYNC_WAIT;
    }
    else if (sync_count > MAX_SYNC_COUNT)
    {
        if (viterbi_cost < STREAM_COST_LIMIT)
        {
            // Sync word missed but we are still decoding a stream reasonably well.
            if (!missing_sync_count) missing_sync_count = 1;
            sync_word_type = M17FrameDecoder::SyncWordType::BERT;
            demodState = DemodState::FRAME;
        }
         else if (missing_sync_count < MAX_MISSING_SYNC)
        {
            missing_sync_count += 1;
            sync_word_type = M17FrameDecoder::SyncWordType::BERT;
            demodState = DemodState::FRAME;
        }
        else
        {
            demodState = DemodState::UNLOCKED;
            dcd.unlock();
        }
    }
}

/**
 * Wait for the sync_count to hit MAX_SYNC_COUNT. This is necessary for
 * proper timing. Otherwise we can be off by 1 byte if the sync arrives
 * a bit early. This may happen due to timing mismatch or noise causing
 * the sync word correlator to trigger early.
 */
template <typename FloatType>
void M17Demodulator<FloatType>::do_sync_wait()
{
    if (sync_count < MAX_SYNC_COUNT)
    {
        sync_count += 1;
        return;
    }
    
    need_clock_update_ = true;
    demodState = DemodState::FRAME;
}

template <typename FloatType>
void M17Demodulator<FloatType>::do_frame(FloatType filtered_sample)
{
    // Only do this when there is no chance of skipping a sample. So do
    // this update as far from the sample point as possible. It should
    // only ever change by +/- 1.
    if (abs(int(sample_index - correlator.index())) == (SAMPLES_PER_SYMBOL / 2))
    {
        clock_recovery.update();
        sample_index = clock_recovery.sample_index();
        return;
    }

    if (correlator.index() != sample_index) return;

    auto sample = filtered_sample - dev.offset();
    sample *= dev.idev();
    sample *= polarity;

    evm.update(sample);

    auto n = llr<FloatType, 4>(sample);
    int8_t* tmp;
    auto len = framer(n, &tmp);
    if (len != 0)
    {
        sync_count = 0;

        M17FrameDecoder::input_buffer_t buffer;
        std::copy(tmp, tmp + len, buffer.begin());
        auto valid = decoder(sync_word_type, buffer, viterbi_cost);

        switch (decoder.state())
        {
        case M17FrameDecoder::State::STREAM:
            demodState = DemodState::STREAM_SYNC;
            break;
        case M17FrameDecoder::State::LSF:
            // If state == LSF, we need to recover LSF from LICH.
            demodState = DemodState::STREAM_SYNC;
            break;
        case M17FrameDecoder::State::BERT:
            demodState = DemodState::BERT_SYNC;
            break;
        default:
            demodState = DemodState::PACKET_SYNC;
            break;
        }

        switch (valid)
        {
        case M17FrameDecoder::DecodeResult::FAIL:
            break;
        case M17FrameDecoder::DecodeResult::OK:
            break;
        case M17FrameDecoder::DecodeResult::INCOMPLETE:
            break;
        }
    }
}

template <typename FloatType>
void M17Demodulator<FloatType>::operator()(const FloatType input)
{
    static int16_t initializing = 1920;

    count_++;

    dcd(input);

    // We need to pump a few ms of data through on startup to initialize
    // the demodulator.
    if (__builtin_expect((initializing), 0))
    {
        --initializing;
        initialize(input);
        count_ = 0;
        return;
    }
 
    if (!dcd_)
    {
        if (count_ % (BLOCK_SIZE * 2) == 0)
        {
            update_dcd();
            dcd.update();
            if (diagnostic_callback)
            {
                diagnostic_callback(int(dcd_), dev.error(), dev.deviation(), dev.offset(), (demodState != DemodState::UNLOCKED),
                    clock_recovery.clock_estimate(), sample_index, sync_sample_index, clock_recovery.sample_index(), viterbi_cost);
            }
            count_ = 0;
        }
        return;
    }

    auto filtered_sample = demod_filter(input);

    correlator.sample(filtered_sample);

    if (correlator.index() == 0)
    {
        if (need_clock_reset_)
        {
            clock_recovery.reset(sync_sample_index);
            need_clock_reset_ = false;
            sample_index = sync_sample_index;
        }
        else if (need_clock_update_) // must avoid update immediately after reset.
        {
            // We have a valid sync word. Update the filter.
            clock_recovery.update(sync_sample_index);
            need_clock_update_ = false;
        }
    }

    // Do this here, after the potential clock recovery reset above.
    clock_recovery(filtered_sample);

    switch (demodState)
    {
    case DemodState::UNLOCKED:
        // In this state, the sample_index is unknown.  We need to find
        // a sync word to find the proper sample_index.  We only leave
        // this state if we believe that we have a valid sample_index.
        do_unlocked();
        break;
    case DemodState::LSF_SYNC:
        do_lsf_sync();
        break;
    case DemodState::STREAM_SYNC:
        do_stream_sync();
        break;
    case DemodState::PACKET_SYNC:
        do_packet_sync();
        break;
    case DemodState::BERT_SYNC:
        do_bert_sync();
        break;
    case DemodState::SYNC_WAIT:
        do_sync_wait();
        break;
    case DemodState::FRAME:
        do_frame(filtered_sample);
        break;
    }

    if (count_ % (BLOCK_SIZE * 5) == 0)
    {
        update_dcd();
        count_ = 0;
        if (diagnostic_callback)
        {
            diagnostic_callback(int(dcd_), evm.evm(), dev.deviation(), dev.offset(), (demodState != DemodState::UNLOCKED),
                clock_recovery.clock_estimate(), sample_index, sync_sample_index, clock_recovery.sample_index(), viterbi_cost);
        }
        dcd.update();
    }
}

} // mobilinkd
