// Copyright 2020 Mobilinkd LLC.
// Copyright 2022 Open Research Institute, Inc.

#include "OPVDemodulator.h"
#include "FirFilter.h"

#include "Numerology.h"
#include <opus/opus.h>

#include <boost/program_options.hpp>
#include <boost/optional.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

const char VERSION[] = "0.1";

using namespace mobilinkd;

bool invert_input = false;
bool quiet = false;
bool debug = false;
bool noise_blanker = false;

OpusDecoder* opus_decoder;

PRBS9 prbs;

struct Config
{
    bool verbose = false;
    bool debug = false;
    bool quiet = false;
    bool invert = false;
    bool noise_blanker = false;

    static std::optional<Config> parse(int argc, char* argv[])
    {
        namespace po = boost::program_options;

        Config result;

        // Declare the supported options.
        po::options_description desc(
            "Program options");
        desc.add_options()
            ("help,h", "Print this help message and exit.")
            ("version,V", "Print the application verion and exit.")
            ("invert,i", po::bool_switch(&result.invert), "invert the received baseband")
            ("noise-blanker,b", po::bool_switch(&result.noise_blanker), "noise blanker -- silence likely corrupt audio")
            ("verbose,v", po::bool_switch(&result.verbose), "verbose output")
            ("debug,d", po::bool_switch(&result.debug), "debug-level output")
            ("quiet,q", po::bool_switch(&result.quiet), "silence all output -- no BERT output")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << "Read OPV baseband from STDIN and write audio to STDOUT\n"
                << desc << std::endl;

            return std::nullopt;
        }

        if (vm.count("version"))
        {
            std::cout << argv[0] << ": " << VERSION << std::endl;
            return std::nullopt;
        }

        try {
            po::notify(vm);
        } catch (std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            std::cout << desc << std::endl;
            return std::nullopt;
        }

        if (result.debug + result.verbose + result.quiet > 1)
        {
            std::cerr << "Only one of quiet, verbose or debug may be chosen." << std::endl;
            return std::nullopt;
        }

        return result;
    }
};

std::optional<Config> config;


bool demodulate_audio(OPVFrameDecoder::stream_type1_bytes_t const& audio, int viterbi_cost)
{
    bool result = true;
    std::array<int16_t, audio_frame_size> buf;
    opus_int32 count;

    if (noise_blanker && viterbi_cost > 80)
    {
        buf.fill(0);
        std::cout.write((const char*)buf.data(), audio_frame_sample_bytes);
        std::cout.write((const char*)buf.data(), audio_frame_sample_bytes);
    }
    else
    {
        bool use_fec = (viterbi_cost > 80); // instead of blanking, use Opus's in-band FEC data if possible

        count = opus_decode(opus_decoder, audio.data(), stream_frame_payload_bytes, buf.data(), audio_frame_size, use_fec);
        if (count != audio_frame_size && config->verbose)
        {
            std::cerr << "Opus decode error, " << count << " bytes, expected " << audio_frame_size << std::endl;
        }
        std::cout.write((const char*)buf.data(), audio_frame_sample_bytes);

       count = opus_decode(opus_decoder, audio.data() + audio_frame_size, stream_frame_payload_bytes, buf.data(), audio_frame_size, use_fec);
        if (count != audio_frame_size && config->verbose)
        {
            std::cerr << "Opus decode error, " << count << " bytes, expected " << audio_frame_size << std::endl;
        }
        std::cout.write((const char*)buf.data(), audio_frame_sample_bytes);
    }

    return result;
}


bool decode_bert(OPVFrameDecoder::stream_type1_bytes_t const& bert_data)
{
    size_t count = 0;

    for (auto b: bert_data)
    {
        for (int i = 0; i != 8; ++i) {
            prbs.validate(b & 0x80);
            b <<= 1;
            count++;
            if (count >= bert_frame_prime_size)
            {
                return true;    // ignore any extra/repeated bits at the end of the frame
            }
        }
    }

    return true;
}


bool handle_frame(OPVFrameDecoder::output_buffer_t const& frame, int viterbi_cost)
{
    using FrameType = OPVFrameDecoder::FrameType;

    bool result = true;

    switch (frame.type)
    {
        case FrameType::OPVOICE:
            result = demodulate_audio(frame.data, viterbi_cost);
            break;
        case FrameType::OPBERT:
            result = decode_bert(frame.data);
            break;
    }

    return result;
}

template <typename FloatType>
void diagnostic_callback(bool dcd, FloatType evm, FloatType deviation, FloatType offset, bool locked,
    FloatType clock, int sample_index, int sync_index, int clock_index, int viterbi_cost)
{
    if (debug) {
        std::cerr << "\rdcd: " << std::setw(1) << int(dcd)
            << ", evm: " << std::setfill(' ') << std::setprecision(4) << std::setw(8) << evm * 100 <<"%"
            << ", deviation: " << std::setprecision(4) << std::setw(8) << deviation
            << ", freq offset: " << std::setprecision(4) << std::setw(8) << offset
            << ", locked: " << std::boolalpha << std::setw(6) << locked << std::dec
            << ", clock: " << std::setprecision(7) << std::setw(8) << clock
            << ", sample: " << std::setw(1) << sample_index << ", "  << sync_index << ", " << clock_index
            << ", cost: " << viterbi_cost;
    }
        
    if (!dcd && prbs.sync()) { // Seems like there should be a better way to do this.
        prbs.reset();
    }

    if (prbs.sync() && !quiet) {
        if (!debug) {
            std::cerr << '\r';
        } else {
            std::cerr << ", ";
        }
    
        auto ber = double(prbs.errors()) / double(prbs.bits());
        char buffer[40];
        snprintf(buffer, 40, "BER: %-1.6lf (%lu bits)", ber, (long unsigned int)prbs.bits());
        std::cerr << buffer;
    }
    std::cerr << std::flush;
}


int main(int argc, char* argv[])
{
    auto config = Config::parse(argc, argv);
    if (!config) return 0;

    invert_input = config->invert;
    quiet = config->quiet;
    debug = config->debug;
    noise_blanker = config->noise_blanker;
    int opus_decoder_err;    // return code from Opus function calls

    opus_decoder = ::opus_decoder_create(audio_sample_rate, 1, &opus_decoder_err);

    using FloatType = float;

    OPVDemodulator<FloatType> demod(handle_frame);

    demod.diagnostics(diagnostic_callback<FloatType>);

    while (std::cin)
    {
        int16_t sample;
        std::cin.read(reinterpret_cast<char*>(&sample), 2);
        if (invert_input) sample *= -1;
        demod(sample / 44000.0);
    }

    std::cerr << std::endl;

    opus_decoder_destroy(opus_decoder);

    return EXIT_SUCCESS;
}
