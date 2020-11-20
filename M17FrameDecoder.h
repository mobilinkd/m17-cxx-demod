// Copyright 2020 Mobilinkd LLC.

#pragma once

#include "M17Randomizer.h"
#include "PolynomialInterleaver.h"
#include "M17Convolution.h"
#include "CRC16.h"
#include "LinkSetupFrame.h"

#include <codec2/codec2.h>

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <iomanip>

namespace mobilinkd
{

struct M17FrameDecoder
{
    M17Randomizer<368> derandomize_;
    PolynomialInterleaver<45, 92, 368> interleaver_;
    CM17Convolution convolution_;
    CRC16<0x5935, 0xFFFF> crc_;

    enum class State {LS_FRAME, LS_LICH, AUDIO};

    State state_ = State::LS_FRAME;

    using buffer_t = std::array<int8_t, 368>;

    using lsf_conv_buffer_t = std::array<uint8_t, 46>;
    using lsf_buffer_t = std::array<uint8_t, 30>;

    using audio_conv_buffer_t = std::array<uint8_t, 34>;
    using audio_buffer_t = std::array<uint8_t, 20>;

    using link_setup_callback_t = std::function<void(audio_buffer_t)>;
    using audio_callback_t = std::function<void(audio_buffer_t)>;
    
    link_setup_callback_t link_setup_callback_;
    audio_callback_t audio_callback_;

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

    void reset() { state_ = State::LS_FRAME; }

    void decode_lsf(buffer_t& buffer)
    {
        lsf_conv_buffer_t cbuffer;
        for (size_t i = 0; i != 368; i += 8)
        {
            uint8_t tmp = 0;
            for (size_t j = 0; j != 8; ++j)
            {
                tmp |= ((buffer[i + j] & 1) << (7 - j));
            }
            cbuffer[i >> 3] = tmp;
        }
        lsf_buffer_t lsf;
        convolution_.decodeLinkSetup(cbuffer.begin(), lsf.begin());
        crc_.reset();
        for (auto x : lsf) crc_(x);
        auto checksum = crc_.get();
        if (checksum == 0) state_ = State::AUDIO;
        else return;
        LinkSetupFrame::encoded_call_t encoded_call;
        std::copy(lsf.begin(), lsf.begin() + 6, encoded_call.begin());
        auto mycall = LinkSetupFrame::decode_callsign(encoded_call);
        std::cerr << "\nTOCALL: ";
        for (auto x : mycall) if (x) std::cerr << x;
        std::cerr << std::endl;

    }

    void demodulate_audio(audio_buffer_t audio)
    {
        std::array<int16_t, 160> buf;
        codec2_decode(codec2, buf.begin(), audio.begin() + 2);
        std::cout.write((const char*)buf.begin(), 320);
        codec2_decode(codec2, buf.begin(), audio.begin() + 10);
        std::cout.write((const char*)buf.begin(), 320);
    }

    void decode_audio(buffer_t& buffer)
    {
        audio_conv_buffer_t cbuffer;
        size_t index = 0;
        for (size_t i = 96; i != 368; i += 8)
        {
            uint8_t tmp = 0;
            for (size_t j = 0; j != 8; ++j)
            {
                tmp |= (buffer[i + j] << (7 - j));
            }
            cbuffer[index++] = tmp;
        }
        audio_buffer_t audio;
        convolution_.decodeData(cbuffer.begin(), audio.begin());
        // for (auto x : audio) std::cout << std::hex << std::setw(2) << std::setfill('0') << int(x) << ' ';
        // std::cout << std::endl;
        crc_.reset();
        for (auto x : audio) crc_(x);
        auto checksum = crc_.get();
        demodulate_audio(audio);
        uint16_t fn = (audio[0] << 8) | audio[1];
        std::cerr << fn << std::endl;
        if (checksum == 0 && fn > 0x7fff) state_ = State::LS_FRAME;
    }

    void operator()(buffer_t& buffer)
    {
        derandomize_(buffer);
        interleaver_.deinterleave(buffer);

        switch(state_)
        {
        case State::LS_FRAME:
            decode_lsf(buffer);
            break;
        case State::LS_LICH:
            state_ = State::LS_FRAME;
            break;
        case State::AUDIO:
            decode_audio(buffer);
            break;
        }
    }
};

} // mobilinkd
