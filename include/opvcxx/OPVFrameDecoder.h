// Copyright 2021 Mobilinkd LLC.
// Copyright 2022 Open Research Institute, Inc.

#pragma once

#include "OPVRandomizer.h"
#include "PolynomialInterleaver.h"
#include "Trellis.h"
#include "Viterbi.h"
#include "OPVFrameHeader.h"
#include "Golay24.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iostream>


namespace mobilinkd
{

struct OPVFrameDecoder
{

    OPVRandomizer<stream_type4_size> derandomize_;
    PolynomialInterleaver<177, 370, stream_type4_size> interleaver_;
    Trellis<4,2> trellis_{makeTrellis<4, 2>({031,027})};
    Viterbi<decltype(trellis_), 4> viterbi_{trellis_};
 
    enum class State { ACQ, STREAM };
    enum class DecodeResult { FAIL, OK, EOS };
    enum class FrameType { OPVOICE, OPBERT };

    State state_ = State::ACQ;      // we're looking to acquire frame sync

    using frame_type4_buffer_t = std::array<int8_t, stream_type4_size>; // scrambled frame excluding sync word

    using encoded_fheader_t = std::array<int8_t, encoded_fheader_size>; // Frame Header (type 2/3)
    using fheader_flags_t = uint32_t;                                   // Flags extracted from frame header

    using stream_type3_buffer_t = std::array<int8_t, stream_type3_payload_size>;    // encoded stream payload
    using stream_type1_buffer_t = std::array<uint8_t, stream_frame_payload_size>;    // decoded stream payload
    using stream_type1_bytes_t = std::array<uint8_t, stream_frame_payload_bytes>;   // decoded stream payload (packed)

    using output_buffer_t = struct {
        FrameType type;
        OPVFrameHeader fheader;
        stream_type1_bytes_t data;
    };


    /**
     * Callback function for frame types.  The caller is expected to return
     * true if the data was good or unknown and false if the data is known
     * to be bad.
     */
    using callback_t = std::function<bool(const output_buffer_t&, int)>;

    callback_t callback_;
    output_buffer_t output_buffer;
    OPVFrameHeader fheader_;

    OPVFrameDecoder(callback_t callback)
    : callback_(callback)
    {}


    void reset()
    {
        state_ = State::ACQ;
    }


    DecodeResult decode_stream(OPVFrameHeader fheader, stream_type3_buffer_t& buffer, size_t& viterbi_cost)
    {
        stream_type1_buffer_t decode_buffer;

        viterbi_cost = viterbi_.decode(buffer, decode_buffer);
        to_byte_array(decode_buffer, output_buffer.data);

        if (fheader.flags & OPVFrameHeader::LAST_FRAME)
        {
            // fputs("\nEOS\n", stderr);
            state_ = State::ACQ;
        }

        output_buffer.type = (fheader.flags & OPVFrameHeader::BERT_MODE) ? FrameType::OPBERT : FrameType::OPVOICE;
        callback_(output_buffer, viterbi_cost);

        return (fheader.flags & OPVFrameHeader::LAST_FRAME) ? DecodeResult::EOS : DecodeResult::OK;
    }


    /**
     * Decode OPV frames.
     * 
     * Before calling this function, we've already detected the sync word for
     * streaming OPV. We get the interleaved and scrambled frame contents
     * (excluding the sync word).
     */
    DecodeResult operator()(frame_type4_buffer_t& buffer, size_t& viterbi_cost)
    {
        encoded_fheader_t encoded_fheader;
        fheader_flags_t fheader_flags;
        stream_type3_buffer_t encoded_payload;
        stream_type1_buffer_t payload;

        derandomize_(buffer);
        interleaver_.deinterleave(buffer);

        std::copy(buffer.begin(), buffer.begin() + encoded_fheader_size, encoded_fheader.begin());
        std::copy(buffer.begin() + encoded_fheader_size, buffer.end(), encoded_payload.begin());

        switch (fheader_.update_frame_header(encoded_fheader))
        {
            case OPVFrameHeader::HeaderResult::FAIL:
                std::cerr << "Failed to decode frame header" << std::endl;
                break;

            case OPVFrameHeader::HeaderResult::UPDATED:
                break;

            case OPVFrameHeader::HeaderResult::NOCHANGE:
            default:
                break;
        }

        return decode_stream(fheader_, encoded_payload, viterbi_cost);
    }
};

} // mobilinkd
