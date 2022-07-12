// Copyright 2020 Mobilinkd LLC.

#include "Util.h"
#include "queue.h"
#include "FirFilter.h"
#include "Trellis.h"
#include "Convolution.h"
#include "PolynomialInterleaver.h"
#include "OPVRandomizer.h"
#include "Util.h"
#include "Golay24.h"
#include "OPVFrameHeader.h"

#include "Numerology.h"
#include <opus/opus.h>

#include <boost/program_options.hpp>
#include <boost/optional.hpp>

#include <thread>

#include <array>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <optional>
#include <mutex>

#include <cstdlib>

#include <signal.h>

// Generated using scikit-commpy
const auto rrc_taps = std::array<double, 150>{
    0.0029364388513841593, 0.0031468394550958484, 0.002699564567597445, 0.001661182944400927, 0.00023319405581230247, -0.0012851320781224025, -0.0025577136087664687, -0.0032843366522956313, -0.0032697038088887226, -0.0024733964729590865, -0.0010285696910973807, 0.0007766690889758685, 0.002553421969211845, 0.0038920145144327816, 0.004451886520053017, 0.00404219185231544, 0.002674727068399207, 0.0005756567993179152, -0.0018493784971116507, -0.004092346891623224, -0.005648131453822014, -0.006126925416243605, -0.005349511529163396, -0.003403189203405097, -0.0006430502751187517, 0.002365929161655135, 0.004957956568090113, 0.006506845894531803, 0.006569574194782443, 0.0050017573119839134, 0.002017321931508163, -0.0018256054303579805, -0.00571615173291049, -0.008746639552588416, -0.010105075751866371, -0.009265784007800534, -0.006136551625729697, -0.001125978562075172, 0.004891777252042491, 0.01071805138282269, 0.01505751553351295, 0.01679337935001369, 0.015256245142156299, 0.01042830577908502, 0.003031522725559901, -0.0055333532968188165, -0.013403099825723372, -0.018598682349642525, -0.01944761739590459, -0.015005271935951746, -0.0053887880354343935, 0.008056525910253532, 0.022816244158307273, 0.035513467692208076, 0.04244131815783876, 0.04025481153629372, 0.02671818654865632, 0.0013810216516704976, -0.03394615682795165, -0.07502635967975885, -0.11540977897637611, -0.14703962203941534, -0.16119995609538576, -0.14969512896336504, -0.10610329539459686, -0.026921412469634916, 0.08757875030779196, 0.23293327870303457, 0.4006012210123992, 0.5786324696325503, 0.7528286479934068, 0.908262741447522, 1.0309661131633199, 1.1095611856548013, 1.1366197723675815, 1.1095611856548013, 1.0309661131633199, 0.908262741447522, 0.7528286479934068, 0.5786324696325503, 0.4006012210123992, 0.23293327870303457, 0.08757875030779196, -0.026921412469634916, -0.10610329539459686, -0.14969512896336504, -0.16119995609538576, -0.14703962203941534, -0.11540977897637611, -0.07502635967975885, -0.03394615682795165, 0.0013810216516704976, 0.02671818654865632, 0.04025481153629372, 0.04244131815783876, 0.035513467692208076, 0.022816244158307273, 0.008056525910253532, -0.0053887880354343935, -0.015005271935951746, -0.01944761739590459, -0.018598682349642525, -0.013403099825723372, -0.0055333532968188165, 0.003031522725559901, 0.01042830577908502, 0.015256245142156299, 0.01679337935001369, 0.01505751553351295, 0.01071805138282269, 0.004891777252042491, -0.001125978562075172, -0.006136551625729697, -0.009265784007800534, -0.010105075751866371, -0.008746639552588416, -0.00571615173291049, -0.0018256054303579805, 0.002017321931508163, 0.0050017573119839134, 0.006569574194782443, 0.006506845894531803, 0.004957956568090113, 0.002365929161655135, -0.0006430502751187517, -0.003403189203405097, -0.005349511529163396, -0.006126925416243605, -0.005648131453822014, -0.004092346891623224, -0.0018493784971116507, 0.0005756567993179152, 0.002674727068399207, 0.00404219185231544, 0.004451886520053017, 0.0038920145144327816, 0.002553421969211845, 0.0007766690889758685, -0.0010285696910973807, -0.0024733964729590865, -0.0032697038088887226, -0.0032843366522956313, -0.0025577136087664687, -0.0012851320781224025, 0.00023319405581230247, 0.001661182944400927, 0.002699564567597445, 0.0031468394550958484, 0.0029364388513841593, 0.0
};

const char VERSION[] = "0.1";

using namespace mobilinkd;

struct Config
{
    std::string source_address;
    std::string audio_device;
    bool verbose = false;
    bool debug = false;
    bool quiet = false;
    bool bitstream = false; // default is baseband audio
    uint32_t bert = 0; // Frames of Bit error rate testing.
    uint64_t token = 0; // authentication token for frame header
    bool invert = false;

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
            ("src,S", po::value<std::string>(&result.source_address)->required(),
                "transmitter identifier (your callsign).")
            ("token,T", po::value<uint64_t>(&result.token)->default_value(0x8765432112345678),
                "authentication token")
            ("audio,a", po::value<std::string>(&result.audio_device),
                "audio device (default is STDIN).")
            ("bitstream,b", po::bool_switch(&result.bitstream),
                "output bitstream (default is baseband).")
            ("bert,B", po::value<uint32_t>(&result.bert)->default_value(0),
                "number of BERT frames to output (default or 0 to read audio from STDIN instead).")
            ("invert,i", po::bool_switch(&result.invert), "invert the output baseband (ignored for bitstream)")
            ("verbose,v", po::bool_switch(&result.verbose), "verbose output")
            ("debug,d", po::bool_switch(&result.debug), "debug-level output")
            ("quiet,q", po::bool_switch(&result.quiet), "silence all output")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << "Read audio from STDIN and write baseband OPV to STDOUT\n"
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

        if (result.source_address.size() > 9)
        {
            std::cerr << "Source identifier too long." << std::endl;
            return std::nullopt;
        }

        return result;
    }
};

std::optional<Config> config;

std::atomic<bool> running{false};

bool invert = false;


// Intercept ^C and just tell the transmit thread to end, which ends the program
void signal_handler(int)
{
    running = false;
    std::cerr << "quitting" << std::endl;
}


// Convert a dibit into a modulation symbol
int8_t bits_to_symbol(uint8_t bits)
{
    switch (bits)
    {
    case 0: return 1;
    case 1: return 3;
    case 2: return -1;
    case 3: return -3;
    }
    abort();
}


// Convert an unpacked array of bits into an unpacked array of modulation symbols
template <typename T, size_t N>
std::array<int8_t, N / 2> bits_to_symbols(const std::array<T, N>& bits)
{
    std::array<int8_t, N / 2> result;
    size_t index = 0;
    for (size_t i = 0; i != N; i += 2)
    {
        result[index++] = bits_to_symbol((bits[i] << 1) | bits[i + 1]);
    }
    return result;
}


// Convert a packed array of bits into an unpacked array of modulation symbols
template <typename T, size_t N>
std::array<int8_t, N * 4> bytes_to_symbols(const std::array<T, N>& bytes)
{
    std::array<int8_t, N * 4> result;
    size_t index = 0;
    for (auto b : bytes)
    {
        for (size_t i = 0; i != 4; ++i)
        {
            result[index++] = bits_to_symbol(b >> 6);
            b <<= 2;
        }
    }
    return result;
}


// Convert an unpacked array of modulation symbols into an array of modulation samples.
// This includes the 10x interpolation using the RRC filter.
template <size_t N>
std::array<int16_t, N*10> symbols_to_baseband(std::array<int8_t, N> symbols)
{
    using namespace mobilinkd;

    static BaseFirFilter<double, std::tuple_size<decltype(rrc_taps)>::value> rrc = makeFirFilter(rrc_taps);

    std::array<int16_t, N*10> baseband;
    baseband.fill(0);
    for (size_t i = 0; i != symbols.size(); ++i)
    {
        baseband[i * 10] = symbols[i];
    }

    for (auto& b : baseband)
    {
        b = rrc(b) * 7168.0 * (invert ? -1.0 : 1.0);
    }

    return baseband;
}


// bitstream_t represents a whole frame worth of unpacked bits (not including sync word)
using bitstream_t = std::array<int8_t, stream_type4_size>;


// output a frame of type4 bits, including the sync word, to cout (packed)
void output_bitstream(std::array<uint8_t, 2> sync_word, const bitstream_t& frame)
{
    for (auto c : sync_word) std::cout << c;        // output the sync word
    for (size_t i = 0; i != frame.size(); i += 8)   // output the fheader and data
    {
        uint8_t c = 0;
        for (size_t j = 0; j != 8; ++j)
        {
            c <<= 1;
            c |= frame[i + j];
        }
        std::cout << c;
    }
}


// output a frame of modulation samples, including the sync word, to cout
void output_baseband(std::array<uint8_t, 2> sync_word, const bitstream_t& frame)
{
    auto sw = bytes_to_symbols(sync_word);
    auto symbols = bits_to_symbols(frame);

    std::array<int8_t, baseband_frame_symbols> temp;
    auto fit = std::copy(sw.begin(), sw.end(), temp.begin());
    std::copy(symbols.begin(), symbols.end(), fit);
    auto baseband = symbols_to_baseband(temp);
    for (auto b : baseband) std::cout << uint8_t(b & 0xFF) << uint8_t(b >> 8);
}


// output a frame, including the sync word, to cout, in the desired format
void output_frame(std::array<uint8_t, 2> sync_word, const bitstream_t& frame)
{
    if (config->bitstream) output_bitstream(sync_word, frame);
    else output_baseband(sync_word, frame);
}


// create and ouput a preamble frame to cout in the desired format
void send_preamble()
{
    // Preamble is simple... bytes -> symbols -> baseband.

    if (config->verbose) std::cerr << "Sending preamble: " << stream_type4_size + 16 << " bits." << std::endl;

    std::array<uint8_t, (stream_type4_size + 16)/8> preamble_bytes;
    preamble_bytes.fill(0x77);  // +3, -3, +3, -3 == 01 11 01 11 == 0x77
    if (config->bitstream)
    {
        for (auto c : preamble_bytes) std::cout << c;
    }
    else // baseband
    {
        auto preamble_symbols = bytes_to_symbols(preamble_bytes);
        auto preamble_baseband = symbols_to_baseband(preamble_symbols);
        for (auto b : preamble_baseband) std::cout << uint8_t(b & 0xFF) << uint8_t(b >> 8);
    }
}

constexpr std::array<uint8_t, 2> STREAM_SYNC_WORD = {0xFF, 0x5D};
constexpr std::array<uint8_t, 2> EOT_SYNC = { 0x55, 0x5D };


// output an end-of-transmission in the desired format
// EOT is just a sync word and enough tail to push it out through the RRC. Not a frame.
void output_eot()
{
    if (config->bitstream)
    {
        for (auto c : EOT_SYNC) std::cout << c;
        for (size_t i = 0; i !=10; ++i) std::cout << '\0'; // Flush the imaginary RRC FIR Filter.
    }
    else // baseband
    {
        std::array<int8_t, 48> out_symbols; // EOT symbols + FIR flush.
        out_symbols.fill(0);    // 0x00 == 00 00 00 00 == +1 +1 +1 +1, why would we send that?
        auto symbols = bytes_to_symbols(EOT_SYNC);  // overwrite the first 8 symbols
        for (size_t i = 0; i != symbols.size(); ++i)
        {
            out_symbols[i] = symbols[i];
        }
        auto baseband = symbols_to_baseband(out_symbols);
        for (auto b : baseband) std::cout << uint8_t(b & 0xFF) << uint8_t(b >> 8);
    }
}


using fheader_t = std::array<uint8_t, fheader_size_bytes>;          // Frame Header (type 1)
using encoded_fheader_t = std::array<int8_t, encoded_fheader_size>; // Frame Header (type 2/3)

using queue_t = queue<int16_t, audio_frame_size>; // the queue can hold up to 40ms worth of PCM audio samples
using audio_frame_t = std::array<int16_t, audio_frame_size>;    // an audio frame is 40ms worth of PCM audio samples
using stream_frame_t = std::array<uint8_t, stream_frame_payload_bytes>; // a stream frame of type1 data bytes
using type3_data_frame_t = std::array<uint8_t, stream_type3_payload_size>;  // a stream frame of type3 bits


// Create the payload for a voice frame, which is made up of two codec frames.
// Caller must ensure that the audio is padded with 0s if the incoming data is incomplete.
stream_frame_t fill_voice_frame(OpusEncoder *opus_encoder, const audio_frame_t& audio)
{
    stream_frame_t result;
    opus_int32 count;

    count  = opus_encode(opus_encoder,
                        const_cast<int16_t*>(&audio[0]), audio_frame_size,
                        &result[0], opus_frame_size_bytes);
    count += opus_encode(opus_encoder,
                        const_cast<int16_t*>(&audio[audio_frame_size]), audio_frame_size,
                        &result[opus_frame_size_bytes], opus_frame_size_bytes);

    if (count != stream_frame_payload_bytes)
    {
        std::cerr << "Got unexpected encoded voice size" << count;
    }

    return result;
}


// Convert a type1 stream frame to type2/type3. That is, convolutional encode it (and puncture if we used puncturing)
type3_data_frame_t encode_stream_frame(const stream_frame_t& payload)
{
    std::array<uint8_t, stream_type2_payload_size> encoded;   // rate-1/2 encoded data bits + 4 flush bits, unpacked
    size_t index = 0;
    uint32_t memory = 0;
    for (auto b : payload)
    {
        for (size_t i = 0; i != 8; ++i)
        {
            uint32_t x = (b & 0x80) >> 7;
            b <<= 1;
            memory = update_memory<4>(memory, x);
            encoded[index++] = convolve_bit(031, memory);
            encoded[index++] = convolve_bit(027, memory);
        }
    }
    // Flush the encoder.
    for (size_t i = 0; i != 4; ++i)
    {
        memory = update_memory<4>(memory, 0);
        encoded[index++] = convolve_bit(031, memory);
        encoded[index++] = convolve_bit(027, memory);
    }

    return encoded;
}


// Create the payload for a BERT frame, exactly the same size as voice frame,
// but filled with bits from the pseudorandom bit sequence generator.
// We use a prime number of bits from the PRBS per frame, so that each frame
// will be unique for a very long while. The rest of the frame is filled up
// with bits from the beginning of the frame, so that they will have the same
// statistics. It's up to the receiver whether those filler bits are counted
// toward the bit error rate.
template <typename PRBS>
stream_frame_t fill_bert_frame(PRBS& prbs)
{
    stream_frame_t bert_bytes;
    std::array<uint8_t, stream_frame_payload_size> bert_bits;
    size_t index = 0;

    for (auto bbit: bert_bits)
    {
        bool bit;

        if (index < bert_frame_prime_size)
        {
            bit = prbs.generate();
        }
        else
        {
            bit = bert_bits[index - bert_frame_prime_size];
        }
        index++;
    }

    to_byte_array(bert_bits, bert_bytes);
    std::cerr << "BERT frame" << std::endl;

    return bert_bytes;
}


void dump_fheader(const fheader_t header)
{
    std::cerr << "Frame Header: "
            << std::hex     // output numbers in hex
            << std::setfill('0');   // fill with 0s

    for (auto hbyte: header)
    {
        std::cerr << std::setw(2) << int(hbyte) << " ";
    }

    //
    if (header[9] & 0x80) std::cerr << "last ";
    if (header[9] & 0x40) std::cerr << "BERT";

    std::cerr << std::endl << std::dec;
}


// Generate the frame header
fheader_t fill_fheader(const std::string& source_callsign, OPVFrameHeader::token_t& access_token, bool is_bert)
{
    fheader_t header;

    header.fill(0);

    OPVFrameHeader::call_t callsign;
    callsign.fill(0);
    std::copy(source_callsign.begin(), source_callsign.end(), callsign.begin());
    auto encoded_callsign = OPVFrameHeader::encode_callsign(callsign);
    uint8_t flags = 0;
    if (is_bert) flags |= 0x40;

    auto it = std::copy(encoded_callsign.begin(), encoded_callsign.end(), header.begin());
    it = std::copy(access_token.begin(), access_token.end(), it);
    header[9] = flags;
    //!!! fill in the rest of the header

    if (config->verbose) dump_fheader(header);

    return header;
}


// Modify the frame header to set the EOS (end of stream) bit
void set_last_frame_bit(fheader_t& fh)
{
    fh[9] |= 0x80;
}


// Encode the frame header with multiple words of Golay 12,24 code.
encoded_fheader_t encode_fheader(fheader_t header)
{
    encoded_fheader_t bits;
    size_t byte_index = 0;
    size_t bit_index = 0;
    uint32_t encoded;

    // Each Golay code spans 1.5 bytes. For convenience, we process them in pairs.
    // Each pair has a first code taking up all of the first byte and half of the second,
    // and a second code taking up the other half of the second byte and all of the third.
    for (size_t byte_index = 0; byte_index < fheader_size_bytes / 3; byte_index += 3)
    {
        encoded = Golay24::encode24(header[byte_index] << 4 | ((header[byte_index+1] >> 4) & 0x0F));
        for (size_t i = 0; i < 24; i++)
        {
            bits[bit_index++] = ((encoded & (1 << 23)) != 0);
            encoded <<= 1;
        }

        encoded = Golay24::encode24((header[byte_index+1] & 0x0F) << 8 | ((header[byte_index+2] >> 4) & 0x0F));
        for (size_t i = 0; i < 24; i++)
        {
            bits[bit_index++] = ((encoded & (1 << 23)) != 0);
            encoded <<= 1;
        }
    }

    return bits;
}


// Combine the fheader with the payload, interleave, randomize, and output the frame
void send_stream_frame(const encoded_fheader_t& fh, const type3_data_frame_t& data)
{

    std::array<int8_t, stream_type4_size> temp;
    auto payload_offset = std::copy(fh.begin(), fh.end(), temp.begin());
    std::copy(data.begin(), data.end(), payload_offset);

    PolynomialInterleaver<45, 92, stream_type4_size> interleaver;   //!!! different interleaver needed?
    OPVRandomizer<stream_type4_size> randomizer;

    interleaver.interleave(temp);
    randomizer.randomize(temp);
    output_frame(STREAM_SYNC_WORD, temp);
}


// Thread function that receives PCM audio samples on a queue and transmits OPV.
// (preamble has already been sent, and fheader has been filled.)
void transmit(queue_t& queue, fheader_t& fh)
{
    int encoder_err;    // return code from Opus function calls

    assert(running);

    auto efh = encode_fheader(fh);
    
    OpusEncoder* opus_encoder = ::opus_encoder_create(audio_sample_rate, 1, OPUS_APPLICATION_VOIP, &encoder_err);

    if (encoder_err < 0)
    {
        std::cerr << "Failed to create an Opus encoder!";
        abort();
    }

    encoder_err = opus_encoder_ctl(opus_encoder, OPUS_SET_BITRATE(opus_bitrate));

    if (encoder_err < 0)
    {
        std::cerr << "Failed to set Opus bitrate!";
        abort();
    }

    encoder_err = opus_encoder_ctl(opus_encoder, OPUS_SET_VBR(0));

    if (encoder_err < 0)
    {
        std::cerr << "Failed to set Opus to constant bit rate mode!";
        abort();
    }

    audio_frame_t audio;
    size_t index = 0;

    while (!queue.is_closed() && queue.empty()) std::this_thread::yield();
    while (!queue.is_closed())
    {
        int16_t sample;
        if (!queue.get(sample, std::chrono::milliseconds(3000))) break; //!!! this could be smarter
        audio[index++] = sample;
        if (index == audio.size())
        {
            index = 0;
            auto type4_data = encode_stream_frame(fill_voice_frame(opus_encoder, audio));
            send_stream_frame(efh, type4_data);
            audio.fill(0);
        } 
    }

    if (index > 0)
    {
        // send partial frame;
        auto type4_data = encode_stream_frame(fill_voice_frame(opus_encoder, audio));
        send_stream_frame(efh, type4_data);
    }

    // Last frame is an extra frame of silence.
    audio.fill(0);
    auto type4_data = encode_stream_frame(fill_voice_frame(opus_encoder, audio));
    set_last_frame_bit(fh);
    if (config->verbose) dump_fheader(fh);
    efh = encode_fheader(fh);
    send_stream_frame(efh, type4_data);
    output_eot();
}


int main(int argc, char* argv[])
{
    using namespace mobilinkd;

    try
    {
        config = Config::parse(argc, argv);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    if (!config) return 0;

    invert = config->invert;

    OPVFrameHeader::token_t access_token;
    std::cerr << "Access token: " << std::hex << std::setw(6) << config->token << std::dec << std::endl;
    access_token[0] = (config->token & 0xff0000) >> 16;
    access_token[1] = (config->token & 0x00ff00) >> 8;
    access_token[2] = (config->token & 0x0000ff);

    auto fh = fill_fheader(config->source_address, access_token, config->bert != 0);  //!!! add parameters
    auto encoded_fh = encode_fheader(fh);

    signal(SIGINT, &signal_handler);

    send_preamble();

    if (!config->bert) {
        running = true;
        queue_t queue;
        std::thread thd([&queue, &fh](){transmit(queue, fh);});

        std::cerr << "opv-mod running. ctrl-D to break." << std::endl;

        // Input must be 8000 SPS, 16-bit LE, 1 channel raw audio.
        while (running)
        {
            int16_t sample;
            if (!std::cin.read(reinterpret_cast<char*>(&sample), 2)) break;
            if (!queue.put(sample, std::chrono::seconds(300))) break;
        }

        running = false;

        queue.close();
        thd.join();
    } else {    // BERT mode
        PRBS9 prbs;

        running = true;
        PolynomialInterleaver<45, 92, stream_type4_size> interleaver;
        OPVRandomizer<stream_type4_size> randomizer;

        uint32_t frame_count;
        for (frame_count = 0; frame_count < config->bert; frame_count++)
        {
            if (!running)
            {
                break;
            }
            // Create a BERT frame of type3 bits
            auto frame = encode_stream_frame(fill_bert_frame(prbs));

            // If this is the last BERT frame, mark it in the frame header
            if (frame_count + 1 == config->bert)
            {
                set_last_frame_bit(fh);
                if (config->verbose) dump_fheader(fh);
                encoded_fh = encode_fheader(fh);
            }

            // Combine with FHeader and make type4 bits
            std::array<int8_t, stream_type4_size> type4_data;
            auto payload_offset = std::copy(encoded_fh.begin(), encoded_fh.end(), type4_data.begin());
            std::copy(frame.begin(), frame.end(), payload_offset);

            interleaver.interleave(type4_data);
            randomizer.randomize(type4_data);
            output_frame(STREAM_SYNC_WORD, type4_data);    
        }

        std::cerr << "Output " << frame_count << " frames of BERT data." << std::endl;
    }

    return EXIT_SUCCESS;
}
