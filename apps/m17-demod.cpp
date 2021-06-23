// Copyright 2020 Mobilinkd LLC.

#include "M17Demodulator.h"
#include "CRC16.h"

#include <codec2/codec2.h>

#include <array>
#include <experimental/array>
#include <iostream>
#include <iomanip>
#include <vector>

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

bool display_lsf = false;
bool invert_input = false;

struct CODEC2 *codec2;

std::vector<uint8_t> current_packet;
size_t packet_frame_counter = 0;
mobilinkd::CRC16<0x1021, 0xFFFF> packet_crc;
mobilinkd::CRC16<0x5935, 0xFFFF> stream_crc;

template <typename T, size_t N>
bool dump_lsf(std::array<T, N> const& lsf)
{
    using namespace mobilinkd;
    
    LinkSetupFrame::encoded_call_t encoded_call;

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

    return true;
}


bool demodulate_audio(mobilinkd::M17FrameDecoder::audio_buffer_t const& audio, int viterbi_cost)
{
    bool result = true;

    std::array<int16_t, 160> buf;
    // First two bytes are the frame counter + EOS indicator.
    if (viterbi_cost < 36 && (audio[0] & 0x80))
    {
        if (display_lsf) std::cerr << "\nEOS" << std::endl;
        result = false;
    }

    codec2_decode(codec2, buf.begin(), audio.begin() + 2);
    std::cout.write((const char*)buf.begin(), 320);
    codec2_decode(codec2, buf.begin(), audio.begin() + 10);
    std::cout.write((const char*)buf.begin(), 320);

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
        
        packet_crc.reset();
        for (auto c : current_packet) packet_crc(c);
        auto checksum = packet_crc.get();

        if (checksum == 0)
        {
            std::cout.write((const char*) &current_packet.front(), current_packet.size());
            return true;
        }

        return false;
    }

    size_t frame_number = (packet_segment[25] & 0x7F) >> 2;
    if (frame_number != packet_frame_counter++)
    {
        std::cerr << "\nPacket frame sequence error\n";
        return false;
    }

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
            current_packet.clear();
            packet_frame_counter = 0;
            result = dump_lsf(frame.lsf);
            break;
        case FrameType::LICH:
            std::cout << "LICH" << std::endl;
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

    M17Demodulator demod(handle_frame);

    demod.verbose(display_diags);

    while (std::cin)
    {
        int16_t sample;
        std::cin.read(reinterpret_cast<char*>(&sample), 2);
        if (invert_input) sample *= -1;
        demod(sample / 44000.0);
    }

    std::cerr << std::endl;

    codec2_destroy(codec2);

    return EXIT_SUCCESS;
}
