// Copyright 2020-2021 Rob Riggs <rob@mobilinkd.com>
// All rights reserved.

#pragma once

#include "ClockRecovery.h"
#include "Correlator.h"
#include "DataCarrierDetect.h"
#include "FirFilter.h"
#include "FreqDevEstimator.h"
#include "OPVFrameDecoder.h"
#include "OPVFramer.h"
#include "Util.h"
#include "Numerology.h"

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
	static constexpr auto rrc_taps = std::array<double, 150>{
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
	};
};

template <>
struct Taps<float>
{
	static constexpr auto rrc_taps = std::array<float, 150>{
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
	};
};

} // detail

template <typename FloatType>
struct OPVDemodulator
{

	static constexpr uint8_t MAX_MISSING_SYNC = 8;
	static constexpr FloatType CORRELATION_NEAR_ZERO = 0.1;		// just to avoid a floating point compare to 0.0

	using correlator_t = Correlator<FloatType>;
	using sync_word_t = SyncWord<correlator_t>;
	using callback_t = OPVFrameDecoder::callback_t;
	using diagnostic_callback_t = std::function<void(bool, FloatType, FloatType, FloatType, bool, FloatType, int, int, int, int)>;

	// In the UNLOCKED state we are expecting to lock onto symbol timing and find a preamble.
	// In the FIRST_SYNC state we are expecting to find a STREAM syncword, but we don't know when.
	// In the STREAM_SYNC state we are expecting to find a STREAM syncword in a small window
	// (specifically, about one frame time after the last one).
	// In the FRAME state we've just passed a syncword window and will process the frame payload.
	// If everything goes perfectly, the normal sequence is:
	// UNLOCKED (some number of times, until preamble detection)
	// FIRST_SYNC (some number of times, until STREAM syncword is detected)
	// FRAME
	// STREAM_SYNC
	// FRAME
	// STREAM_SYNC
	// FRAME
	// ...
	enum class DemodState { UNLOCKED, FIRST_SYNC, STREAM_SYNC, FRAME };

	BaseFirFilter<FloatType, detail::Taps<FloatType>::rrc_taps.size()> demod_filter{detail::Taps<FloatType>::rrc_taps};
	DataCarrierDetect<FloatType, sample_rate, 500> dcd{10000, 15500, 1.0, 4.0};	//!!! may need to revise these values
	ClockRecovery<FloatType, sample_rate, symbol_rate> clock_recovery;

	correlator_t correlator;
	sync_word_t preamble_sync{{+3,-3,+3,-3,+3,-3,+3,-3}, 29.f};		// accept only positive correlation
	sync_word_t stream_sync{{-3,-3,-3,-3,+3,+3,-3,+3}, 32.f};		// accept only positive correlation

	FreqDevEstimator<FloatType> dev;
	FloatType idev;
	size_t count_ = 0;

	int8_t polarity = 1;
	OPVFramer<stream_type4_size> framer;
	OPVFrameDecoder decoder;
	DemodState demodState = DemodState::UNLOCKED;
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

	OPVDemodulator(callback_t callback)
	: decoder(callback)
	{}

	virtual ~OPVDemodulator() {}

	void dcd_on();
	void dcd_off();
	void initialize(const FloatType input);
	void update_dcd();
	void do_unlocked();
	void do_first_sync();
	void do_stream_sync();
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
void OPVDemodulator<FloatType>::update_values(uint8_t index)
{
	correlator.apply([this,index](FloatType t){dev.sample(t);}, index);
	dev.update();
	sync_sample_index = index;
}

template <typename FloatType>
void OPVDemodulator<FloatType>::dcd_on()
{
	// Data carrier newly detected.
	dcd_ = true;
	sync_count = 0;
	missing_sync_count = 0;

	dev.reset();
	framer.reset();
	decoder.reset();
}

template <typename FloatType>
void OPVDemodulator<FloatType>::dcd_off()
{
	// Just lost data carrier.
	dcd_ = false;
	demodState = DemodState::UNLOCKED;
	std::cerr << "DCD lost at sample " << debug_sample_count << std::endl;	//!!! debug
}

template <typename FloatType>
void OPVDemodulator<FloatType>::initialize(const FloatType input)
{
	auto filtered_sample = demod_filter(input);
	correlator.sample(filtered_sample);
}

template <typename FloatType>
void OPVDemodulator<FloatType>::update_dcd()
{
	if (!dcd_ && dcd.dcd())
	{
		// fputs("\nAOS\n", stderr);
		dcd_on();
		need_clock_reset_ = true;
	}
	else if (dcd_ && !dcd.dcd())
	{
		// fputs("\nLOS\n", stderr);
		dcd_off();
	}
}

template <typename FloatType>
void OPVDemodulator<FloatType>::do_unlocked()
{
	// We expect to find the preamble immediately after DCD.
	if (missing_sync_count < samples_per_frame)
	{
		missing_sync_count += 1;
		auto sync_index = preamble_sync(correlator);
		auto sync_updated = preamble_sync.updated();
		if (sync_updated)
		{
			std::cerr << "\nDetected preamble at sample " << debug_sample_count << std::endl;	//!!! debug
			sync_count = 0;
			missing_sync_count = 0;
			need_clock_reset_ = true;
			dev.reset();
			update_values(sync_index);
			sample_index = sync_index;
			demodState = DemodState::FIRST_SYNC;	// now looking for a stream sync word
		}
		return;
	}

	// We didn't find preamble; check for the STREAM syncword in case we're joining in the middle
	auto sync_index = stream_sync(correlator);
	auto sync_updated = stream_sync.updated();
	if (sync_updated)
	{
		std::cerr << "Stream sync detected while unlocked at sample " << debug_sample_count << std::endl; //!!! debug

		sync_count = 0;
		missing_sync_count = 0;
		need_clock_reset_ = true;
		dev.reset();
		update_values(sync_index);
		sample_index = sync_index;
		demodState = DemodState::FRAME;
		return;
	}
}


// Search for the first STREAM syncword. We have symbol timing but not frame timing.
// We may still be in the preamble; this is normal at the start of a transmission.
// If we joined mid-transmission, any preamble detections will be false hits, which
// aren't very common but are not especially unusual. Either way, we will keep
// looking until we've matched the syncword (success), or until we've seen a frame worth of
// symbols that match neither preamble nor the STREAM syncword (fail back to UNLOCKED).
template <typename FloatType>
void OPVDemodulator<FloatType>::do_first_sync()
{
	FloatType sync_triggered;	//!!! no need to initialize = 0.;

	if (correlator.index() != sample_index) return;	// We already have symbol timing, we can skip non-peak samples.

	std::cerr << "FIRST sample " << debug_sample_count << std::endl;	//!!! debug

	// We'll check for preamble first. The order doesn't really matter, since the chances
	// of matching both preamble and the STREAM syncword are zero.
	sync_triggered = preamble_sync.triggered(correlator);
	if (sync_triggered > CORRELATION_NEAR_ZERO)
	{
		std::cerr << "Seeing preamble at sample " << debug_sample_count << std::endl;	//!!! debug
		return;		// Seeing preamble; keep looking. Don't count this as a sync miss.
	}

	// Now check for the STREAM syncword.
	sync_triggered = stream_sync.triggered(correlator);
	if (sync_triggered > CORRELATION_NEAR_ZERO)
	{
		// Found the STREAM syncword. Now we have frame timing and can process frames.
		std::cerr << "\nDetected first STREAM sync word at sample " << debug_sample_count << std::endl; //!!! debug
		missing_sync_count = 0;
		need_clock_update_ = true;
		update_values(sample_index);
		demodState = DemodState::FRAME;
	}
	else
	{
		// Didn't find preamble or STREAM syncword; count these and check if we've had too many.
		// Normally there should be only one frame of preamble, and a syncword every frame thereafter,
		// so if we go a frame (or so) without seeing the syncword, we've failed.
		// Even if we had accurate timing when we entered this state, timing may have drifted off, so
		// we declare failure as soon as we can.
		//!!! It might be better to keep a symbol timing tracking loop running all the time.
		if (++missing_sync_count > baseband_frame_symbols)
		{
			std::cerr << "FAILED to find first syncword by sample " << debug_sample_count << std::endl;	//!! debug
			demodState = DemodState::UNLOCKED;
			missing_sync_count = 0;
		}
		else
		{
			// We haven't found the syncword yet, but we're still looking.
			// Just keep the deviation tracker fed. Timing doesn't change.
			update_values(sample_index);
		}
	}
}


// Search for a STREAM sync word after the first one. In this case we have both
// symbol timing and frame timing. We expect to see the STREAM sync word in the
// nominal location, exactly one frame time after the previous one. If we detect
// the STREAM sync word, we use it to refresh our timing synchronization.
// If we don't detect the STREAM sync word, that's OK for a while. We just go on
// as if we had. But if that happens too many times, we assume that we've lost
// synchronization with the signal.
//!!! It's possible we could do something smarter, maybe trust the Golay codes
//!!! in the fheader to validate a longer freewheeling period. Or maybe it'd
//!!! just be better to have a live symbol tracking loop.
template <typename FloatType>
void OPVDemodulator<FloatType>::do_stream_sync()
{
	uint8_t sync_index = stream_sync(correlator);
	int8_t sync_updated = stream_sync.updated();
	sync_count += 1;
	if (sync_updated)
	{
		missing_sync_count = 0;
		if (sync_count > 70)	// sample 71 is the first that's nominally in the last symbol of the sync word
		{
			std::cerr << "\nDetected STREAM sync word at sample " << debug_sample_count << std::endl; //!!! debug
			update_values(sync_index);
			demodState = DemodState::FRAME;
		}
		return;
	}
	else if (sync_count > 87)	// sample 87 is the latest we'll accept a sync word detection as matching
	{
		update_values(sync_index);
		missing_sync_count += 1;
		if (missing_sync_count < MAX_MISSING_SYNC)
		{
			std::cerr << "\nFaking a STREAM sync word " << missing_sync_count << " at sample " << debug_sample_count << std::endl; //!!! debug
			demodState = DemodState::FRAME;
		}
		else
		{
			std::cerr << "\nDone faking sync words at sample " << debug_sample_count << std::endl;	//!! debug
			// fputs("\n!SYNC\n", stderr);
			demodState = DemodState::FIRST_SYNC;
		}
	}
}


// Process a frame.
// We have frame timing, thanks to the STREAM syncword. Either we just detected one, or else
// we are freewheeling based on an older (but still recent) syncword detection.
template <typename FloatType>
void OPVDemodulator<FloatType>::do_frame(FloatType filtered_sample)
{
	if (correlator.index() != sample_index) return;	// we have symbol timing; no need to process non-peak samples

	static uint8_t cost_count = 0;

	// Correct the input sample (representing an input symbol) for estimated deviation magnitude, offset, and polarity.
	auto sample = filtered_sample - dev.offset();
	sample *= dev.idev();
	sample *= polarity;

	// Convert the corrected symbol (FloatType) into its LLR representation.
	auto llr_symbol = llr<FloatType, 4>(sample);

	// Feed these LLR symbols into the OPVFramer. It will gather them up into a frame buffer,
	// converting from symbols to bits, and returning nonzero (the frame length in bits) only
	// when the buffer is full.
	int8_t* framer_buffer_ptr;
	auto len = framer(llr_symbol, &framer_buffer_ptr);
	if (len != 0)
	{
//		std::cerr << "Framer returned " << len << " at sample " << debug_sample_count << std::endl;
		assert(len == stream_type4_size);

		need_clock_update_ = true;

		OPVFrameDecoder::frame_type4_buffer_t buffer;
		std::copy(framer_buffer_ptr, framer_buffer_ptr + len, buffer.begin());
		auto frame_decode_result = decoder(buffer, viterbi_cost);

		cost_count = viterbi_cost > 90 ? cost_count + 1 : 0;
		cost_count = viterbi_cost > 100 ? cost_count + 1 : cost_count;
		cost_count = viterbi_cost > 110 ? cost_count + 1 : cost_count;

		if (cost_count > 75)
		{
			std::cerr << "Viterbi cost high too long at sample " << debug_sample_count << std::endl;	//!!! debug
			cost_count = 0;
			demodState = DemodState::UNLOCKED;
			// fputs("\nCOST\n", stderr);
			return;
		}

		sync_count = 0;

		switch (frame_decode_result)
		{
		case OPVFrameDecoder::DecodeResult::EOS:
			std::cerr << "EOS at sample " << debug_sample_count << std::endl;	//!!! debug
			//!!! EOS is just a hint to upper layers; here's where we'd pass it up somehow.

			// It's OK for a new stream to start immediately without a new preamble.
			//!!! should be quick to drop out of lock if we don't detect an immediately next frame
			demodState = DemodState::STREAM_SYNC;
			break;
		case OPVFrameDecoder::DecodeResult::OK:
			demodState = DemodState::STREAM_SYNC;	// Expect a new STREAM sync word next
			break;
		}
	}
}

template <typename FloatType>
void OPVDemodulator<FloatType>::operator()(const FloatType input)
{
	static int16_t initializing = samples_per_frame;
	static bool initialized = false; //!!! debug

	// std::cerr << "Sample " << debug_sample_count << ": " << input << std::endl;	//!!! debug

	count_++;

	dcd(input);

	// We need to pump a few ms of data through on startup to initialize
	// the demodulator.
	if (initializing) // [[unlikely]]
	{
		--initializing;
		initialize(input);
		count_ = 0;
		return;
	}

	if (! initialized) std::cerr << "Initialize complete at sample " << debug_sample_count << std::endl;	//!!! debug
	initialized = true;//!!! debug

	if (!dcd_)
	{
		if (count_ % (baseband_frame_symbols * 2) == 0)
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

//	std::cerr << "@ " << debug_sample_count << " filtered_sample = " << filtered_sample << std::endl;	//!!!debug
	correlator.sample(filtered_sample);

	if (correlator.index() == 0)
	{
		if (need_clock_reset_)
		{
			clock_recovery.reset();
			need_clock_reset_ = false;
		}
		else if (need_clock_update_) // must avoid update immediately after reset.
		{
			clock_recovery.update();
			uint8_t clock_index = clock_recovery.sample_index();
			uint8_t clock_diff = std::abs(sample_index - clock_index);
			uint8_t sync_diff = std::abs(sample_index - sync_sample_index);
			bool clock_diff_ok = clock_diff <= 1 || clock_diff == 9;
			bool sync_diff_ok = sync_diff <= 1 || sync_diff == 9;
			if (clock_diff_ok) sample_index = clock_index;
			else if (sync_diff_ok) sample_index = sync_sample_index;
			// else unchanged.
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
	case DemodState::FIRST_SYNC:
		do_first_sync();
		break;
	case DemodState::STREAM_SYNC:
		do_stream_sync();
		break;
	case DemodState::FRAME:
		do_frame(filtered_sample);
		break;
	}

	if (count_ % (baseband_frame_symbols * 5) == 0)
	{
		update_dcd();
		count_ = 0;
		if (diagnostic_callback)
		{
			diagnostic_callback(int(dcd_), dev.error(), dev.deviation(), dev.offset(), (demodState != DemodState::UNLOCKED),
				clock_recovery.clock_estimate(), sample_index, sync_sample_index, clock_recovery.sample_index(), viterbi_cost);
		}
		dcd.update();
	}
}

} // mobilinkd
