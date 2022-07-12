// Copyright 2020 Mobilinkd LLC.
// Copyright 2022 Open Research Institute, Inc.

#pragma once

#include "Golay24.h"
#include "Numerology.h"

#include <array>
#include <cstdint>
#include <string_view> // Don't have std::span in C++17.
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace mobilinkd
{

struct OPVFrameHeader
{
    using call_t = std::array<char,10>;             // NUL-terminated C-string.
    using encoded_call_t = std::array<uint8_t, 6>;
    using token_t = std::array<uint8_t, 3>;
    using flags_t = uint32_t;   // only 24 LSbits are sent
    using raw_fheader_t = std::array<uint8_t, fheader_size_bytes>;
    using encoded_fheader_t = std::array<int8_t, encoded_fheader_size>; // Frame Header (type 2/3)

    static constexpr flags_t LAST_FRAME = 0x800000;
    static constexpr flags_t BERT_MODE  = 0x400000;

    enum class HeaderResult { FAIL, UPDATED, NOCHANGE };

    raw_fheader_t raw_fheader_ = {0};   // Undecoded bytes of frame header
    call_t callsign = {0};     // Source callsign claimed by sender
    token_t token = {0};       // authentication token offered by sender
    flags_t flags = 0U;        // Flags set by sender

    /**
     * The callsign is encoded in base-40 starting with the right-most
     * character.  The final value is written out in "big-endian" form, with
     * the most-significant value first.  This leads to 0-padding of shorter
     * callsigns.
     *
     * @param[in] callsign is the callsign to encode.
     * @param[in] strict is a flag (disabled by default) which indicates whether
     *  invalid characters are allowed and assugned a value of 0 or not allowed,
     *  resulting in an exception.
     * @return the encoded callsign as an array of 6 bytes.
     * @throw invalid_argument when strict is true and an invalid callsign (one
     *  containing an unmappable character) is passed.
     */
    static encoded_call_t encode_callsign(call_t callsign, bool strict = false)
    {
        // Encode the characters to base-40 digits.
        uint64_t encoded = 0;

        std::reverse(callsign.begin(), callsign.end());

        for (auto c : callsign)
        {
            encoded *= 40;
            if (c >= 'A' and c <= 'Z')
            {
                encoded += c - 'A' + 1;
            }
            else if (c >= '0' and c <= '9')
            {
                encoded += c - '0' + 27;
            }
            else if (c == '-')
            {
                encoded += 37;
            }
            else if (c == '/')
            {
                encoded += 38;
            }
            else if (c == '.')
            {
                encoded += 39;
            }
            else if (strict)
            {
                throw std::invalid_argument("bad callsign");
            }
        }
        const auto p = reinterpret_cast<uint8_t*>(&encoded);

        encoded_call_t result;
        std::copy(p, p + 6, result.rbegin());
        
        return result;
    }

    /**
     * Decode a base-40 encoded callsign to its text representation.  This decodes
     * a 6-byte big-endian value into a string of up to 9 characters.
     */
    static call_t decode_callsign(encoded_call_t callsign, bool strict = false)
    {
        static const char callsign_map[] = "xABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/.";

        call_t result;

        uint64_t encoded = 0;       // This only works on little endian architectures.
        auto p = reinterpret_cast<uint8_t*>(&encoded);
        std::copy(callsign.rbegin(), callsign.rend(), p);

        // decode each base-40 digit and map them to the appropriate character.
        result.fill(0);
        size_t index = 0;
        while (encoded)
        {
            result[index++] = callsign_map[encoded % 40];
            encoded /= 40;
        }

        return result;
    }


    OPVFrameHeader& myCall(const char*)
    {
        return *this;
    }


    // Initialize/update the frame header info from a received frame header.
    // Any failure to decode a Golay24 codeword will abort this procedure.  !!! this could be smarter
    HeaderResult update_frame_header(encoded_fheader_t efh)
    {
        uint32_t received, decoded;
        raw_fheader_t raw_fh;
        std::array<uint8_t, fheader_size_bytes * 2> nibbles;
        encoded_call_t  call;
        HeaderResult result = HeaderResult::NOCHANGE;

        // For convenience, we'll decode into an array of nibbles (4 bits each)
        // initially and then group them up into bytes afterwards.
        for (size_t i = 0; i < fheader_size_bytes * 2; i += 3)
        {
            received = ((efh[i+0] << 16) & 0xff0000) | ((efh[i+1] << 8) & 0x00ff00) | (efh[i+2] & 0x0000ff);
            if (! Golay24::decode(received, decoded)) return HeaderResult::FAIL;
            nibbles[i+0] = (decoded >> 8) & 0x0f;
            nibbles[i+1] = (decoded >> 4) & 0x0f;
            nibbles[i+2] = (decoded >> 0) & 0x0f;
        }

        for (size_t i = 0; i < fheader_size_bytes; i++)
        {
            raw_fh[i] = nibbles[2*i] << 4 + nibbles[2*i + 1];
        }

        // If the callsign (after Golay decoding but before callsign decoding) has changed,
        // decode and store the updated callsign
        if (! std::equal(raw_fh.begin(), raw_fh.begin() + 6, raw_fheader_.begin()))
        {
            result = HeaderResult::UPDATED;
            std::copy(raw_fh.begin(), raw_fh.begin() + 6, call.begin());
            callsign = decode_callsign(call);
            std::cerr << "Callsign: ";
            for (auto x : callsign) if (x) std::cerr << x;
        }

        // If the decoded authentication token has changed, store it
        if (! std::equal(raw_fh.begin() + 6, raw_fh.begin() + 9, raw_fheader_.begin() + 6))
        {
            result = HeaderResult::UPDATED;
            std::copy(raw_fh.begin() + 6, raw_fh.begin() + 9, token.begin());
            std::cerr << "Token: " << std::hex << token[0] << token[1] << token[2] << std::dec;
        }

        // If the decoded flags have changed, store them
        if (! std::equal(raw_fh.begin() + 18, raw_fh.end(), raw_fheader_.begin() + 18))
        {
            result = HeaderResult::UPDATED;
            flags = ((raw_fh[9] << 16) & 0xff0000) | ((raw_fh[10] << 8) & 0x00ff00) | (raw_fh[11] & 0x0000ff);
            std::cerr << "Flags: " << std::hex << flags << std::dec;
        }

        if (result == HeaderResult::UPDATED)
        {
            std::cerr << std::endl;
            std::copy(raw_fh.begin(), raw_fh.end(), raw_fheader_.begin());
        }

        return result;
    }
};

} // mobilinkd
