// Copyright 2021 Mobilinkd LLC.

#pragma once

#include "M17Randomizer.h"
#include "PolynomialInterleaver.h"
#include "Trellis.h"
#include "Viterbi.h"
#include "CRC16.h"
#include "LinkSetupFrame.h"
#include "Golay24.h"

#include <codec2/codec2.h>

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <iomanip>
#include <vector>

extern bool display_lsf;

namespace mobilinkd
{

namespace detail {

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

} // detail
struct M17FrameDecoder
{
    M17Randomizer<368> derandomize_;
    PolynomialInterleaver<45, 92, 368> interleaver_;
    Trellis<4,2> trellis_{makeTrellis<4, 2>({031,027})};
    Viterbi<decltype(trellis_), 4> viterbi_{trellis_};
    CRC16<0x5935, 0xFFFF> crc_;
    CRC16<0x1021, 0xFFFF> packet_crc_;
 
    enum class State {LSF, STREAM, BASIC_PACKET, FULL_PACKET};
    enum class SyncWordType { LSF, STREAM, PACKET, RESERVED };
    enum class DecodeResult { FAIL, OK, EOS, INCOMPLETE, PACKET_INCOMPLETE };

    State state_ = State::LSF;

    using buffer_t = std::array<int8_t, 368>;

    using lsf_conv_buffer_t = std::array<uint8_t, 46>;
    using lsf_buffer_t = std::array<uint8_t, 30>;

    using audio_conv_buffer_t = std::array<uint8_t, 34>;
    using audio_buffer_t = std::array<uint8_t, 20>;

    using link_setup_callback_t = std::function<void(audio_buffer_t)>;
    using audio_callback_t = std::function<void(audio_buffer_t)>;
    
    link_setup_callback_t link_setup_callback_;
    audio_callback_t audio_callback_;
    lsf_buffer_t current_lsf;
    std::array<int8_t, 6> lich_buffer;
    uint8_t lich_segments{0};       ///< one bit per received LICH fragment.
    std::vector<uint8_t> current_packet;
    size_t packet_frame_counter = 0;
    bool passall_ = false;

    struct CODEC2 *codec2;

    M17FrameDecoder(
        link_setup_callback_t link_setup_callback = link_setup_callback_t(),
        audio_callback_t audio_callback = audio_callback_t()
    )
    : link_setup_callback_(link_setup_callback)
    , audio_callback_(audio_callback)
    {
        codec2 = ::codec2_create(CODEC2_MODE_3200);
    }

    ~M17FrameDecoder()
    {
        codec2_destroy(codec2);
    }

    void update_state(std::array<uint8_t, 240>& lsf_output)
    {
        if (lsf_output[111]) // LSF type bit 0
        {
            state_ = State::STREAM;
        }
        else    // packet frame comes next.
        {
            uint8_t packet_type = (lsf_output[109] << 1) | lsf_output[110];

            if (!current_packet.empty())
            {
                std::cerr << "Incomplete packet found" << std::endl;
                current_packet.clear();
            }

            packet_frame_counter = 0;

            switch (packet_type)
            {
            case 1: // RAW -- ignore LSF.
                state_ = State::BASIC_PACKET;
                break;
            case 2: // ENCAPSULATED
                state_ = State::FULL_PACKET;
                packet_frame_counter = 0;
                detail::append_packet(current_packet, lsf_output);
                break;
            default:
                std::cerr << "LSF for reserved packet type" << std::endl;
                state_ = State::FULL_PACKET;
                packet_frame_counter = 0;
                detail::append_packet(current_packet, lsf_output);
            }
        }
    }


    void reset() { state_ = State::LSF; }

    void dump_lsf(const lsf_buffer_t& lsf)
    {
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
    }


    /**
     * Decode the LSF and, if it is valid, transition to the next state.
     *
     * The LSF is returned for STREAM mode, dropped for BASIC_PACKET mode,
     * and captured for FULL_PACKET mode.
     *
     * @param buffer
     * @param lsf
     * @param ber
     * @return
     */
    DecodeResult decode_lsf(buffer_t& buffer, size_t& ber)
    {
        std::array<uint8_t, 240> output;
        auto dp = depunctured<488>(P1, buffer);
        ber = viterbi_.decode(dp, output);
        ber = ber > 60 ? ber - 60 : 0;
        current_lsf = to_byte_array(output);
        crc_.reset();
        for (auto c : current_lsf) crc_(c);
        auto checksum = crc_.get();

        if (checksum == 0)
        {
            update_state(output);
            if (state_ == State::STREAM)
            {
                if (display_lsf) dump_lsf(current_lsf);
                return DecodeResult::OK;
            }
            return DecodeResult::OK;
        }
        else
        {
            lich_segments = 0;
            current_lsf.fill(0);
            return DecodeResult::FAIL;
        }
    }

    // Unpack  & decode LICH fragments into tmp_buffer.
    bool unpack_lich(buffer_t& buffer)
    {
        size_t index = 0;
        // Read the 4 24-bit codewords from LICH
        for (size_t i = 0; i != 4; ++i) // for each codeword
        {
            uint32_t codeword = 0;
            for (size_t j = 0; j != 24; ++j) // for each bit in codeword
            {
                codeword <<= 1;
                codeword |= (buffer[i * 24 + j] > 0);
            }
            uint32_t decoded = 0;
            if (!Golay24::decode(codeword, decoded))
            {
                return false;
            }
            decoded >>= 12; // Remove check bits and parity.
            // append codeword.
            if (i & 1)
            {
                lich_buffer[index++] |= (decoded >> 8);     // upper 4 bits
                lich_buffer[index++] = (decoded & 0xFF);    // lower 8 bits
            }
            else
            {
                lich_buffer[index++] |= (decoded >> 4);     // upper 8 bits
                lich_buffer[index] = (decoded & 0x0F) << 4; // lower 4 bits
            }
        }
        return true;
    }

    DecodeResult decode_lich(buffer_t& buffer, size_t& ber)
    {
        lich_buffer.fill(0);
        // Read the 4 12-bit codewords from LICH into buffers.lich.
        if (!unpack_lich(buffer)) return DecodeResult::FAIL;

        uint8_t fragment_number = lich_buffer[5];   // Get fragment number.
        fragment_number = (fragment_number >> 5) & 7;

        // Copy decoded LICH to superframe buffer.
        std::copy(lich_buffer.begin(), lich_buffer.begin() + 5,
            current_lsf.begin() + (fragment_number * 5));

        lich_segments |= (1 << fragment_number);        // Indicate segment received.
        if (lich_segments != 0x3F) return DecodeResult::INCOMPLETE;        // More to go...

        crc_.reset();
        for (auto c : current_lsf) crc_(c);
        auto checksum = crc_.get();
        // INFO("LICH crc = %04x", checksum);
        if (checksum == 0)
        {
        	lich_segments = 0;
            state_ = State::STREAM;
            ber = 0;
            if (display_lsf) dump_lsf(current_lsf);
            return DecodeResult::OK;
        }

        // Failed CRC... try again.
        // lich_segments = 0;
        current_lsf.fill(0);
        ber = 128;
        return DecodeResult::INCOMPLETE;
    }

    void demodulate_audio(audio_buffer_t audio)
    {
        std::array<int16_t, 160> buf;
        codec2_decode(codec2, buf.begin(), audio.begin() + 2);
        std::cout.write((const char*)buf.begin(), 320);
        codec2_decode(codec2, buf.begin(), audio.begin() + 10);
        std::cout.write((const char*)buf.begin(), 320);
    }

    DecodeResult decode_stream(buffer_t& buffer, size_t& ber)
    {
        std::array<int8_t, 272> tmp;
        std::copy(buffer.begin() + 96, buffer.end(), tmp.begin());
        std::array<uint8_t, 160> output;
        auto dp = depunctured<328>(P2, tmp);
        ber = viterbi_.decode(dp, output) - 28;
        auto audio = to_byte_array(output);
        crc_.reset();
        for (auto x : audio) crc_(x);
        auto checksum = crc_.get();
        if ((checksum == 0) && (audio[0] & 0x80))
        {
            if (display_lsf) std::cerr << "\nEOS" << std::endl; 
            state_ = State::LSF;
        }

        demodulate_audio(audio);
        return state_ == State::LSF ? DecodeResult::EOS : DecodeResult::OK;
    }

    /**
     * Capture packet frames until an EOF bit is found.  The raw packet is
     * returned without the checksum.
     *
     * @pre current_packet is not null.

     * @param buffer the demodulated M17 symbols in LLR format.
     * @param ber the estimated BER (really more SNR) of the packet.
     * @return true if a valid packet is returned, otherwise false.
     */
    DecodeResult decode_basic_packet(buffer_t& buffer, size_t& ber)
    {
        std::array<int8_t, 420> dp;
        auto bit_count = depuncture(buffer, dp, P3);
        std::array<uint8_t, 206> output;
        ber = viterbi_.decode(dp, output);
        auto packet_segment = to_byte_array(output);

        if (packet_segment[25] & 0x80) // last frame of packet.
        {
            size_t packet_size = (packet_segment[25] & 0x7F) >> 2;
            packet_size = std::min(packet_size, size_t(25));
            for (size_t i = 0; i != packet_size; ++i)
            {
                current_packet.push_back(packet_segment[i]);
            }
            packet_frame_counter = 0;
            state_ = State::LSF;
            
            packet_crc_.reset();
            for (auto c : current_lsf) packet_crc_(c);
            auto checksum = packet_crc_.get();

            if (checksum == 0)
            {
                std::cout.write((const char*) &current_packet.front(), current_packet.size());
                current_packet.clear();
                return DecodeResult::OK;
            }

            // WARN("packet bad fcs = %04x, crc = %04x", current_packet->fcs(), current_packet->crc());
            if (passall_)
            {
                std::cout.write((const char*) &current_packet.front(), current_packet.size());
            }

            current_packet.clear();
            return DecodeResult::FAIL;
        }

        size_t frame_number = (packet_segment[25] & 0x7F) >> 2;
        if (frame_number != packet_frame_counter++)
        {
            std::cerr << "\nPacket frame sequence error\n";
        }

        for (size_t i = 0; i != 25; ++i)
        {
            current_packet.push_back(packet_segment[i]);
        }

        return DecodeResult::PACKET_INCOMPLETE;
    }

    /**
     * Decode full packet types.  The packet is returned without checking
     * the CRC.
     *
     * @pre current_packet is not empty and contains a link setup frame.
     *
     * @param buffer
     * @param ber
     * @return
     */
    DecodeResult decode_full_packet(buffer_t& buffer, size_t& ber)
    {
        std::array<int8_t, 420> dp;
        auto bit_count = depuncture(buffer, dp, P3);
        std::array<uint8_t, 206> output;
        ber = viterbi_.decode(dp, output);
        auto packet_segment = to_byte_array(output);

        if (packet_segment[25] & 0x80) // last packet;
        {
            size_t packet_size = (packet_segment[25] & 0x7F) >> 2;
            packet_size = std::min(packet_size, size_t(25));
            for (size_t i = 0; i != packet_size; ++i)
            {
                current_packet.push_back(packet_segment[i]);
            }

            std::cout.write((const char*)&current_packet.front(), current_packet.size());

            current_packet.clear();
            packet_frame_counter = 0;
            state_ = State::LSF;
            return DecodeResult::OK;
        }

        size_t frame_number = (packet_segment[25] & 0x7F) >> 2;
        if (frame_number != packet_frame_counter++)
        {
            std::cerr << "Packet frame sequence error" << std::endl;
        }

        for (size_t i = 0; i != 25; ++i)
        {
            current_packet.push_back(packet_segment[i]);
        }

        return DecodeResult::PACKET_INCOMPLETE;
    }

    /**
     * Decode M17 frames.  The decoder uses the sync word to determine frame
     * type and to update its state machine.
     *
     * The decoder receives M17 frame type indicator (based on sync word) and
     * frames from the M17 demodulator.
     *
     * If the frame is an LSF, the state immediately changes to LSF. When
     * in LSF mode, the state machine can transition to:
     *
     *  - LSF if the CRC is bad.
     *  - STREAM if the LSF type field indicates Stream.
     *  - BASIC_PACKET if the LSF type field indicates Packet and the packet
     *    type is RAW.
     *  - FULL_PACKET if the LSF type field indicates Packet and the packet
     *    type is ENCAPSULATED or RESERVED.
     *
     * When in LSF mode, if an LSF frame is received it is parsed as an LSF.
     * When a STREAM frame is received, it attempts to recover an LSF from
     * the LICH.  PACKET frame types are ignored when state is LSF.
     *
     * When in STREAM mode, the state machine can transition to either:
     *
     *  - STREAM when a any stream frame is received.
     *  - LSF when the EOS indicator is set, or when a packet frame is received.
     *
     * When in BASIC_PACKET mode, the state machine can transition to either:
     *
     *  - BASIC_PACKET when any packet frame is received.
     *  - LSF when the EOS indicator is set, or when a stream frame is received.
     *
     * When in FULL_PACKET mode, the state machine can transition to either:
     *
     *  - FULL_PACKET when any packet frame is received.
     *  - LSF when the EOS indicator is set, or when a stream frame is received.
     */
    DecodeResult operator()(SyncWordType frame_type, buffer_t& buffer, size_t& ber)
    {
        derandomize_(buffer);
        interleaver_.deinterleave(buffer);

        // This is out state machined.
        switch(frame_type)
        {
        case SyncWordType::LSF:
            state_ = State::LSF;
            return decode_lsf(buffer, ber);
        case SyncWordType::STREAM:
            switch (state_)
            {
            case State::LSF:
                decode_lich(buffer, ber);
            case State::STREAM:
                return decode_stream(buffer, ber);
            default:
                state_ = State::LSF;
            }
            break;
        case SyncWordType::PACKET:
            switch (state_)
            {
            case State::BASIC_PACKET:
                return decode_basic_packet(buffer, ber);
            case State::FULL_PACKET:
                return decode_full_packet(buffer, ber);
            default:
                state_ = State::LSF;
            }
            break;
        case SyncWordType::RESERVED:
            state_ = State::LSF;
            break;
        }

        return DecodeResult::FAIL;
    }

    State state() const { return state_; }
};

} // mobilinkd
