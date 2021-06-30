// Copyright 2020 Mobilinkd LLC.

#include "M17Demodulator.h"
#include "CRC16.h"
#include "ax25_frame.h"
#include "FirFilter.h"

#include <codec2/codec2.h>
#include <boost/crc.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>


bool display_lsf = false;
bool invert_input = false;

struct CODEC2 *codec2;

std::vector<uint8_t> current_packet;
size_t packet_frame_counter = 0;
mobilinkd::CRC16<0x1021, 0xFFFF> packet_crc;
mobilinkd::CRC16<0x5935, 0xFFFF> stream_crc;

template <typename T, size_t N>
std::vector<uint8_t> to_packet(std::array<T, N> in)
{
    std::vector<uint8_t> result;
    result.reserve(N/8);

    uint8_t out = 0;
    size_t b = 0;

    for (auto c : in)
    {
        out = (out << 1) | c;
        if (++b == 8)
        {
            result.push_back(out);
            out = 0;
            b = 0;
        }
    }

    return result;
}

template <typename T, size_t N>
void append_packet(std::vector<uint8_t>& result, std::array<T, N> in)
{
    uint8_t out = 0;
    size_t b = 0;

    for (auto c : in)
    {
        out = (out << 1) | c;
        if (++b == 8)
        {
            result.push_back(out);
            out = 0;
            b = 0;
        }
    }
}

template <typename T, size_t N>
bool dump_lsf(std::array<T, N> const& lsf)
{
    using namespace mobilinkd;
    
    LinkSetupFrame::encoded_call_t encoded_call;

    if (display_lsf)
    {
        std::copy(lsf.begin() + 6, lsf.begin() + 12, encoded_call.begin());
        auto src = LinkSetupFrame::decode_callsign(encoded_call);
        std::cerr << "\nSRC: ";
        for (auto x : src) if (x) std::cerr << x;

        std::copy(lsf.begin(), lsf.begin() + 6, encoded_call.begin());
        auto dest = LinkSetupFrame::decode_callsign(encoded_call);
        std::cerr << ", DEST: ";
        for (auto x : dest) if (x) std::cerr << x;

        uint16_t type = (lsf[12] << 8) | lsf[13];
        std::cerr << ", TYPE: " << std::setw(4) << std::setfill('0') << type;

        std::cerr << ", NONCE: ";
        for (size_t i = 14; i != 28; ++i) std::cerr << std::hex << std::setw(2) << std::setfill('0') << int(lsf[i]);

        uint16_t crc = (lsf[28] << 8) | lsf[29];
        std::cerr << ", CRC: " << std::setw(4) << std::setfill('0') << crc;
        std::cerr << std::dec << std::endl;
    }

    current_packet.clear();
    packet_frame_counter = 0;

    if (!lsf[111]) // LSF type bit 0
    {
        uint8_t packet_type = (lsf[109] << 1) | lsf[110];

        switch (packet_type)
        {
        case 1: // RAW -- ignore LSF.
             break;
        case 2: // ENCAPSULATED
            append_packet(current_packet, lsf);
            break;
        default:
            std::cerr << "LSF for reserved packet type" << std::endl;
            append_packet(current_packet, lsf);
        }
    }

    return true;
}


bool demodulate_audio(mobilinkd::M17FrameDecoder::audio_buffer_t const& audio, int viterbi_cost)
{
    bool result = true;

    std::array<int16_t, 160> buf;
    // First two bytes are the frame counter + EOS indicator.
    if (viterbi_cost < 70 && (audio[0] & 0x80))
    {
        if (display_lsf) std::cerr << "\nEOS" << std::endl;
        result = false;
    }

    if (viterbi_cost < 70)
    {
        codec2_decode(codec2, buf.begin(), audio.begin() + 2);
        std::cout.write((const char*)buf.begin(), 320);
        codec2_decode(codec2, buf.begin(), audio.begin() + 10);
        std::cout.write((const char*)buf.begin(), 320);
    }
    else
    {
        buf.fill(0);
        std::cout.write((const char*)buf.begin(), 320);
        std::cout.write((const char*)buf.begin(), 320);
    }

    return result;
}

bool decode_packet(mobilinkd::M17FrameDecoder::packet_buffer_t const& packet_segment)
{
    if (packet_segment[25] & 0x80) // last frame of packet.
    {
        size_t packet_size = (packet_segment[25] & 0x7F) >> 2;
        packet_size = std::min(packet_size, size_t(25));
        for (size_t i = 0; i != packet_size; ++i)
        {
            current_packet.push_back(packet_segment[i]);
        }
        
        boost::crc_optimal<16, 0x1021, 0xFFFF, 0xFFFF, true, true> crc;
        crc.process_bytes(&current_packet.front(), current_packet.size());
        uint16_t checksum = crc.checksum();

        if (checksum == 0x0f47)
        {
            std::string ax25;
            ax25.reserve(current_packet.size());
            for (auto c : current_packet) ax25.push_back(char(c));
            mobilinkd::ax25_frame frame(ax25);
            std::cerr << '\n';
            mobilinkd::write(std::cerr, frame);
            return true;
        }

        std::cerr << "\nPacket checksum error: " << std::hex << checksum << std::dec << std::endl;

        return false;
    }

    size_t frame_number = (packet_segment[25] & 0x7F) >> 2;
    if (frame_number != packet_frame_counter)
    {
        std::cerr << "\nPacket frame sequence error. Got " << frame_number << ", expected " << packet_frame_counter << "\n";
        return false;
    }

    packet_frame_counter += 1;

    for (size_t i = 0; i != 25; ++i)
    {
        current_packet.push_back(packet_segment[i]);
    }

    return true;
}


bool decode_full_packet(mobilinkd::M17FrameDecoder::packet_buffer_t const& packet_segment)
{
    if (packet_segment[25] & 0x80) // last packet;
    {
        size_t packet_size = (packet_segment[25] & 0x7F) >> 2;
        packet_size = std::min(packet_size, size_t(25));
        for (size_t i = 0; i != packet_size; ++i)
        {
            current_packet.push_back(packet_segment[i]);
        }

        std::cout.write((const char*)&current_packet.front(), current_packet.size());

        return true;
    }

    size_t frame_number = (packet_segment[25] & 0x7F) >> 2;
    if (frame_number != packet_frame_counter++)
    {
        std::cerr << "Packet frame sequence error" << std::endl;
        return false;
    }

    for (size_t i = 0; i != 25; ++i)
    {
        current_packet.push_back(packet_segment[i]);
    }

    return true;
}

bool handle_frame(mobilinkd::M17FrameDecoder::output_buffer_t const& frame, int viterbi_cost)
{
    using FrameType = mobilinkd::M17FrameDecoder::FrameType;

    bool result = true;

    switch (frame.type)
    {
        case FrameType::LSF:
            result = dump_lsf(frame.lsf);
            break;
        case FrameType::LICH:
            std::cerr << "LICH" << std::endl;
            break;
        case FrameType::STREAM:
            result = demodulate_audio(frame.stream, viterbi_cost);
            break;
        case FrameType::BASIC_PACKET:
            result = decode_packet(frame.packet);
            break;
        case FrameType::FULL_PACKET:
            result = decode_packet(frame.packet);
            break;
    }

    return result;
}

template <typename FloatType>
void diagnostic_callback(bool dcd, FloatType evm, FloatType deviation, FloatType offset, bool locked,
    FloatType clock, int sample_index, int sync_index, int clock_index, int viterbi_cost)
{
    std::cerr << "\rdcd: " << std::setw(1) << int(dcd)
        << ", evm: " << std::setfill(' ') << std::setprecision(4) << std::setw(8) << evm * 100 <<"%"
        << ", deviation: " << std::setprecision(4) << std::setw(8) << deviation
        << ", freq offset: " << std::setprecision(4) << std::setw(8) << offset
        << ", locked: " << std::boolalpha << std::setw(6) << locked << std::dec
        << ", clock: " << std::setprecision(7) << std::setw(8) << clock
        << ", sample: " << std::setw(1) << sample_index << ", "  << sync_index << ", " << clock_index
        << ", cost: " << viterbi_cost << "         "  << std::ends;
}


int main(int argc, char* argv[])
{
    using namespace mobilinkd;
    using namespace std::string_literals;

    bool display_diags = false;
    for (size_t i = 1; i != argc; ++i)
    {
        if (argv[i] == "-d"s) display_diags = true;
        if (argv[i] == "-l"s) display_lsf = true;
        if (argv[i] == "-i"s) invert_input = true;

    }

    codec2 = ::codec2_create(CODEC2_MODE_3200);

    using FloatType = float;

    M17Demodulator<FloatType> demod(handle_frame);

#if 0
    if (display_diags)
    {
        std::cerr << "Size of M17Demodulator: " << sizeof(demod) << std::endl;
        std::cerr << "    Size of M17FrameDecoder: " << sizeof(M17FrameDecoder) << std::endl;
        std::cerr << "        Size of M17Randomizer<368>: " << sizeof(M17Randomizer<368>) << std::endl;
        std::cerr << "        Size of PolynomialInterleaver<45, 92, 368>: " << sizeof(PolynomialInterleaver<45, 92, 368>) << std::endl;
        std::cerr << "        Size of Trellis<4,2>: " << sizeof(Trellis<4,2>) << std::endl;
        std::cerr << "        Size of Viterbi<Trellis<4,2>, 4>: " << sizeof(Viterbi<Trellis<4,2>, 4>) << std::endl;
        std::cerr << "        Size of output_buffer_t: " << sizeof(M17FrameDecoder::output_buffer_t) << std::endl;
        std::cerr << "        Size of depunctured_buffer_t: " << sizeof(M17FrameDecoder::depunctured_buffer_t) << std::endl;
        std::cerr << "        Size of decode_buffer_t: " << sizeof(M17FrameDecoder::decode_buffer_t) << std::endl;
        std::cerr << "    Size of M17 Matched Filter: " << sizeof(BaseFirFilter<FloatType, detail::Taps<double>::rrc_taps.size()>) << std::endl;
        std::cerr << "    Size of M17 Correlator: " << sizeof(Correlator<FloatType>) << std::endl;
        std::cerr << "    Size of M17 SyncWord: " << sizeof(SyncWord<Correlator<FloatType>>) << std::endl;
        std::cerr << "    Size of M17 DataCarrierDetect: " << sizeof(DataCarrierDetect<FloatType, 48000, 500>) << std::endl;
        std::cerr << "    Size of M17 ClockRecovery: " << sizeof(ClockRecovery<FloatType, 48000, 4800>) << std::endl;
        std::cerr << "    Size of M17 M17Framer: " << sizeof(M17Framer<368>) << std::endl;
    }
#endif

    demod.diagnostics(diagnostic_callback<FloatType>);

    // std::ofstream out("stream.out");
    // auto old_rdbuf = std::clog.rdbuf();
    // std::clog.rdbuf(out.rdbuf());

    while (std::cin)
    {
        int16_t sample;
        std::cin.read(reinterpret_cast<char*>(&sample), 2);
        if (invert_input) sample *= -1;
        demod(sample / 44000.0);
    }

    std::cerr << std::endl;

    codec2_destroy(codec2);

    // std::clog.rdbuf(old_rdbuf);

    return EXIT_SUCCESS;
}
