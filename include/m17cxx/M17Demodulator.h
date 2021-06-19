// Copyright 2020-2021 Rob Riggs <rob@mobilinkd.com>
// All rights reserved.

#pragma once

#include "ClockRecovery.h"
#include "Correlator.h"
#include "DataCarrierDetect.h"
#include "FreqDevEstimator.h"
#include "M17FrameDecoder.h"
#include "M17Framer.h"
#include "SymbolEvm.h"
#include "Util.h"

#include <algorithm>
#include <array>
#include <optional>
#include <tuple>

extern bool display_lsf;

namespace mobilinkd {

namespace detail
{
inline const auto rrc_taps = std::experimental::make_array<double>(
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

} // detail

struct M17Demodulator
{
    static constexpr uint32_t SAMPLE_RATE = 48000;
    static constexpr uint32_t SYMBOL_RATE = 4800;
    static constexpr uint32_t SAMPLES_PER_SYMBOL = SAMPLE_RATE / SYMBOL_RATE;
    static constexpr uint16_t BLOCK_SIZE = 192;

    static constexpr float sample_rate = SAMPLE_RATE;
    static constexpr float symbol_rate = SYMBOL_RATE;

    static constexpr uint8_t MAX_MISSING_SYNC = 5;

	using collelator_t = Correlator<double>;
    using sync_word_t = SyncWord<collelator_t>;

    enum class DemodState { UNLOCKED, LSF_SYNC, STREAM_SYNC, PACKET_SYNC, FRAME };

    BaseFirFilter<double, std::tuple_size<decltype(detail::rrc_taps)>::value> demod_filter = makeFirFilter(detail::rrc_taps);
    DataCarrierDetect<double, SAMPLE_RATE, 1000> dcd{2000, 4000, 5.0};
    ClockRecovery<float, SAMPLE_RATE, SYMBOL_RATE> clock_recovery;

    collelator_t correlator;
    sync_word_t preamble_sync{{+3,-3,+3,-3,+3,-3,+3,-3}, 29.f};
    sync_word_t lsf_sync{{+3,+3,+3,+3,-3,-3,+3,-3}, 32.f, -31.f};
    sync_word_t packet_sync{{3,-3,3,3,-3,-3,-3,-3}, 31.f};

    FreqDevEstimator<double> dev;
	double idev;
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
	bool verbose_ = false;
    size_t ber = 0;
    int sync_count = 0;
    int missing_sync_count = 0;

    virtual ~M17Demodulator() {}

    void dcd_on();
    void dcd_off();
    void initialize(const double input);
    void update_dcd();
    void do_unlocked();
    void do_lsf_sync();
    void do_packet_sync();
    void do_stream_sync();
    void do_frame(float filtered_sample);

    bool locked() const
    {
        return dcd_;
    }

    void passall(bool enabled)
    {
        passall_ = enabled;
        // decoder.passall(enabled);
    }

	void verbose(bool value)
	{
		verbose_ = value;
	}

    void update_values(uint8_t index);

    void operator()(const double input);
};

void M17Demodulator::update_values(uint8_t index)
{
	correlator.apply([this,index](float t){dev.sample(t);}, index);
	dev.update();
	sample_index = index;
}

void M17Demodulator::dcd_on()
{
	// Data carrier newly detected.
	dcd_ = true;
	sync_count = 0;
	missing_sync_count = 0;

	dev.reset();
    framer.reset();
    decoder.reset();
}

void M17Demodulator::dcd_off()
{
	// Just lost data carrier.
	dcd_ = false;
	demodState = DemodState::UNLOCKED;
}

void M17Demodulator::initialize(const double input)
{
	auto filtered_sample = demod_filter(input);
	correlator.sample(filtered_sample);
}

void M17Demodulator::update_dcd()
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

void M17Demodulator::do_unlocked()
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
			update_values(sync_index);
			demodState = DemodState::LSF_SYNC;
		}
	}
	else // Otherwise we start searching for a sync word.
	{
		auto sync_index = lsf_sync(correlator);
		auto sync_updated = lsf_sync.updated();
		if (sync_updated)
		{
			sync_count = 0;
			missing_sync_count = 0;
			need_clock_reset_ = true;
			update_values(sync_index);
			dev.reset();
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
	}
}

/**
 * Check for LSF sync word.  We only enter the DemodState::LSF_SYNC state
 * if a preamble sync has been detected, which also means that sample_index
 * has been initialized to a sane value for the baseband.
 */
void M17Demodulator::do_lsf_sync()
{
	double sync_triggered = 0.;

	if (correlator.index() == sample_index)
	{
    	sync_triggered = preamble_sync.triggered(correlator);
		if (sync_triggered != 0)
		{
			update_values(sample_index);
			return;
		}
		sync_triggered = lsf_sync.triggered(correlator);
		if (sync_triggered != 0)
		{
			missing_sync_count = 0;
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
			demodState = DemodState::UNLOCKED;
			missing_sync_count = 0;
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
void M17Demodulator::do_stream_sync()
{
	uint8_t sync_index = lsf_sync(correlator);
	int8_t sync_updated = lsf_sync.updated();
	sync_count += 1;
	if (sync_updated < 0)   // Stream sync word
	{
		missing_sync_count = 0;
		if (sync_count <= 70)
		{
			update_values(sample_index);
			sync_word_type = M17FrameDecoder::SyncWordType::STREAM;
			demodState = DemodState::FRAME;
		}
		else if (sync_count > 70)
		{
			update_values(sample_index);
			sync_word_type = M17FrameDecoder::SyncWordType::STREAM;
			demodState = DemodState::FRAME;
		}
		return;
	}
    else if (sync_count > 87)
    {
		dev.update();
    	missing_sync_count += 1;
    	if (missing_sync_count < MAX_MISSING_SYNC)
    	{
			sync_word_type = M17FrameDecoder::SyncWordType::STREAM;
			demodState = DemodState::FRAME;
    	}
    	else
    	{
			demodState = DemodState::LSF_SYNC;
    	}
    }
}

/**
 * Check for a packet sync word.  DemodState::PACKET_SYNC can only be
 * entered from a valid LSF frame decode with the data/packet type bit set.
 */
void M17Demodulator::do_packet_sync()
{
	auto sync_index = packet_sync(correlator);
	auto sync_updated = packet_sync.updated();
	sync_count += 1;
	if (sync_count > 70 && sync_updated)
	{
		missing_sync_count = 0;
		update_values(sample_index);
		sync_word_type = M17FrameDecoder::SyncWordType::PACKET;
		demodState = DemodState::FRAME;
	}
	else if (sync_count > 87)
	{
		missing_sync_count += 1;
    	if (missing_sync_count < MAX_MISSING_SYNC)
    	{
			sync_word_type = M17FrameDecoder::SyncWordType::PACKET;
			demodState = DemodState::FRAME;
    	}
    	else
    	{
			demodState = DemodState::UNLOCKED;
    	}
	}
}


[[gnu::noinline]]
void M17Demodulator::do_frame(float filtered_sample)
{
	if (correlator.index() != sample_index) return;

	auto sample = filtered_sample - dev.offset();
	sample *= dev.idev();
    sample *= polarity;

	auto n = llr<double, 4>(sample);
	int8_t* tmp;
	auto len = framer(n, &tmp);
	if (len != 0)
	{
		need_clock_update_ = true;

		M17FrameDecoder::buffer_t buffer;
		std::copy(tmp, tmp + len, buffer.begin());
		auto valid = decoder(sync_word_type, buffer, ber);

		switch (decoder.state())
		{
		case M17FrameDecoder::State::STREAM:
			demodState = DemodState::STREAM_SYNC;
			break;
		case M17FrameDecoder::State::LSF:
			// If state == LSF, we need to recover LSF from LICH.
			demodState = DemodState::STREAM_SYNC;
			break;
		default:
			demodState = DemodState::PACKET_SYNC;
			break;
		}

		sync_count = 0;

		switch (valid)
		{
		case M17FrameDecoder::DecodeResult::FAIL:
			break;
		case M17FrameDecoder::DecodeResult::EOS:
			demodState = DemodState::LSF_SYNC;
			std::cerr << "\nEOS\n";
			break;
		case M17FrameDecoder::DecodeResult::OK:
			break;
		case M17FrameDecoder::DecodeResult::INCOMPLETE:
			break;
		}
	}
}

void M17Demodulator::operator()(const double input)
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
		if (count_ % BLOCK_SIZE == 0)
		{
			update_dcd();
			dcd.update();
			if (verbose_)
			{
				std::cerr << "\rdcd: " << std::setw(1) << int(dcd_)
					<< ", evm: " << std::setfill(' ') << std::setprecision(4) << std::setw(8) << dev.error() * 100 <<"%"
					<< ", deviation: " << std::setprecision(4) << std::setw(8) << dev.deviation()
					<< ", freq offset: " << std::setprecision(4) << std::setw(8) << dev.offset()
					<< ", locked: " << std::boolalpha << std::setw(6) << (demodState != DemodState::UNLOCKED)
					<< ", clock: " << std::setprecision(7) << std::setw(8) << clock_recovery.clock_estimate()
					<< ", sample: " << std::setw(1) << int(sample_index)
					<< ", ber: " << ber << "         "  << std::ends;
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
			clock_recovery.reset();
			need_clock_reset_ = false;
		}
		if (need_clock_update_)
		{
			clock_recovery.update();
			sample_index = clock_recovery.sample_index();
			need_clock_update_ = false;
		}
	}

	clock_recovery(filtered_sample);

	if (demodState != DemodState::UNLOCKED && correlator.index() == sample_index)
	{
		dev.sample(filtered_sample);
	}

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
	case DemodState::FRAME:
		do_frame(filtered_sample);
		break;
	}

	if (count_ % (BLOCK_SIZE * 5) == 0)
	{
		update_dcd();
		count_ = 0;
		if (verbose_)
		{
			std::cerr << "\rdcd: " << std::setw(1) << int(dcd_)
				<< ", evm: " << std::setfill(' ') << std::setprecision(4) << std::setw(8) << dev.error() * 100 <<"%"
				<< ", deviation: " << std::setprecision(4) << std::setw(8) << dev.deviation()
				<< ", freq offset: " << std::setprecision(4) << std::setw(8) << dev.offset()
				<< ", locked: " << std::boolalpha << std::setw(6) << (demodState != DemodState::UNLOCKED)
				<< ", clock: " << std::setprecision(7) << std::setw(8) << clock_recovery.clock_estimate()
				<< ", sample: " << std::setw(1) << int(sample_index)
				<< ", ber: " << ber << "         "  << std::ends;
		}
    	dcd.update();
	}
}

} // mobilinkd
