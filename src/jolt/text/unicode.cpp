#include <jolt/memory/allocator.hpp>
#include "unicode.hpp"

using namespace jolt::memory;

/**
 * UTF-8 decoding table.
 *
 * The table is divided into two sections, each of 12 entries. The first 12 entries are for input 0,
 * the second 12 entries are for input 1. Each n-th entry for a given input represents the
 * hard-coded behavioural information for the automaton at a given step. The automaton has therefore
 * 12 possible states with a single binary input.
 *
 * The structure of each entry is, given the current state and input value, as follows:
 * Bits 0..6 is the next state.
 * Bits 8..15 is the mask to apply to the input character to produce the next input value.
 */
static const uint16_t utf8_dec_tbl[] = {
  0x7f00, 2, 2, 0x1fe4, 2, 0x3f00, 0x0fe7, 2, 0x3fe4, 0x07ea, 2, 0x3fe7, 1, 3, 2, 6, 5, 2, 9, 8, 2, 2, 11, 2};

/**
 * UTF-8 encoding table.
 *
 * The table is divided into two sections, each of 5 entries. The first 5 entries are for input 0,
 * the second 5 entries are for input 1. Each n-th entry for a given input represents the hard-coded
 * behavioural information for the automaton at a given step. The automaton has therefore 5 possible
 * states with a single binary input.
 *
 * The structure of each entry is, given the current state and input value, as follows:
 * Bits 0..3 is the next state.
 * Bits 4..7 is the number of bits to shift to encode the metadata bits in the UTF characters.
 * Bits 8..15 next value for the `m` internal parameter.
 */
static const uint16_t utf8_enc_tbl[] = {
  0x0870, 0x0850, 0x0840, 0x0830, 0x0004, 0x0b61, 0x1062, 0x1663, 0x0064, 0x0004};

namespace jolt {
    namespace text {
        size_t utf8_len(const utf8c *s, size_t const s_sz) {
            const utf8c *const s_end = s + s_sz;
            size_t s_len = 0;

            while(s && s < s_end) {
                s = utf8_next_cp(s, s_end - s);
                ++s_len;
            }

            return s_len;
        }

        bool utf8_is_sane(const utf8c *s, size_t const s_sz) {
            const utf8c *const s_end = s + s_sz;

            while(s && s < s_end) {
                utf32c value_out = 0;
                uint16_t decode_state = UTF8_DECODE_STATE_INIT;
                uint8_t next_state;

                do {
                    s += JLT_UTF_DECODE_STATE_INC_AMOUNT(decode_state);
                    decode_state = utf8_decode_cp(*s, &value_out, decode_state);
                    next_state = JLT_UTF_DECODE_NEXT_STATE(decode_state);
                } while(next_state != 0 && next_state != 2);

                if(next_state == UTF8_DECODE_STATE_ERROR) {
                    return false;
                }

                s = utf8_next_cp(s, s_end - s);
            }

            return true;
        }

        inline uint16_t utf8_decode_cp(const utf8c in, utf32c *const out, uint16_t const state) {
            uint8_t const k = static_cast<uint8_t>(state >> 8); // State upper byte
            uint8_t const v = (in & k) != 0;                    // Value of input bit
            uint16_t const x = utf8_dec_tbl[(state & 0x7F) + (12 & (static_cast<uint8_t>(0) - v))];
            uint8_t const next_state = static_cast<uint8_t>(x & 0x000F);
            uint8_t const mask = static_cast<uint8_t>((x & 0xFF00) >> 8);
            uint8_t const shift = static_cast<uint8_t>((x & 0x0070) >> 4);
            uint8_t const inc = static_cast<uint8_t>((x & 0x0080) >> 7);

            *out = (*out | (in & mask)) << shift;

            return static_cast<uint16_t>(next_state | inc << 7)
                   | choose(static_cast<uint16_t>(0x100), static_cast<uint16_t>(k), inc) << 7;
        }

        void utf8_decode(const utf8c *sin, size_t const sin_len, utf32c *sout, size_t const sout_len) {
            const utf8c *const sin_begin = sin;
            const utf8c *const sin_end = sin + sin_len;
            const utf32c *const sout_end = sout + sout_len;

            while(sin && sin < sin_end && sout < sout_end) {
                utf32c value_out = 0;
                uint16_t decode_state = UTF8_DECODE_STATE_INIT;
                uint8_t next_state;

                do {
                    sin += JLT_UTF_DECODE_STATE_INC_AMOUNT(decode_state);
                    decode_state = utf8_decode_cp(*sin, &value_out, decode_state);
                    next_state = JLT_UTF_DECODE_NEXT_STATE(decode_state);
                } while(next_state != 0 && next_state != 2);

                if(next_state == UTF8_DECODE_STATE_SUCCESS) {
                    *(sout++) = value_out;
                    sin = utf8_next_cp(sin, sin_end - sin);
                } else { // Error
                    *(sout++) = UNICODE_CP_REPLACEMENT;
                    utf8c *sin_cp_begin = utf8_cp_start(sin, sin_begin);

                    if(sin != sin_cp_begin) {
                        sin = utf8_next_cp(sin, sin_end - sin);
                    }
                }
            }
        }

        inline utf8c *utf8_next_cp(const utf8c *s, long long len) {
            if(len) {
                do {
                    ++s;
                    len--;
                } while((*s & 0xc0) == 0x80 && len > 0);
            }

            return len ? const_cast<utf8c *>(s) : nullptr;
        }

        inline utf8c *utf8_cp_start(const utf8c *s, const utf8c *const s_begin) {
            utf8c c;

            for(c = (*s & 0xc0); c == 0x80 && s != s_begin; c = (*(--s) & 0xc0)) {}

            return c != 0x80 ? const_cast<utf8c *>(s) : nullptr;
        }

        inline uint16_t utf8_encode_cp(const utf32c in, utf8c *const out, uint16_t const state) {
            uint8_t const k = (state >> 5) & 0x07;   // Current state
            uint8_t const tot_shifts = state & 0x1f; // Total amount of shifted bits
            uint8_t const m = state >> 8;            // Number of bits to shift to determine next state
            utf32c const c = 0xffff'ffff >> m << m;  // Mask to determine next state
            uint8_t const v = (in & c) != 0;         // Automaton input to determine next state
            uint16_t const x = utf8_enc_tbl[k + (5 & (8 - v))]; // Next state word
            uint8_t const next_state = x & 0x000F;
            uint8_t const num_shifts = (x & 0x00F0) >> 4; // Number of bits to shift in next step
            uint8_t const m_next = x >> 8;
            uint8_t const next_tot_shifts = tot_shifts + num_shifts;

            out[k] = (0xff << (num_shifts + 1))
                     | static_cast<uint8_t>((in & ((1UL << (next_tot_shifts + 1)) - 1)) >> tot_shifts);

            return (next_state << 5) | next_tot_shifts | (m_next << 8);
        }

        size_t utf8_encode(const utf32c *sin, size_t const sin_len, utf8c *sout, size_t const sout_len) {
            const utf32c *const sin_end = sin + sin_len;
            const utf8c *const sout_end = sout + sout_len;
            size_t tot_bytes_out = 0;

            while(sin < sin_end && sout < sout_end) {
                utf8c value_out[4];
                uint16_t state = UTF8_ENCODE_STATE_INIT;
                uint8_t result;
                int nc = 0;

                do {
                    ++nc;
                    state = utf8_encode_cp(*sin, value_out, state);
                    result = JLT_UTF_ENCODE_NEXT_STATE(state);
                } while(result && result != 4);

                if(result) {
                    // Error
                    nc = sizeof(UTF8_CP_REPLACEMENT) / sizeof(UTF8_CP_REPLACEMENT[0]);

                    for(int i = 0; i < nc && sout < sout_end; ++i) { *(sout++) = UTF8_CP_REPLACEMENT[i]; }
                } else {
                    for(int i = nc - 1; i >= 0 && sout < sout_end; --i) { *(sout++) = value_out[i]; }
                }

                tot_bytes_out += nc;
                ++sin;
            }

            return tot_bytes_out;
        }

        size_t utf8_encode_buffer_len(const utf32c *sin, size_t const sin_len) {
            const utf32c *const sin_end = sin + sin_len;
            size_t tot_bytes_out = 0;

            while(sin < sin_end) {
                utf8c value_out[4];
                uint16_t state = UTF8_ENCODE_STATE_INIT;
                uint8_t result;
                size_t nc = 0;

                do {
                    ++nc;
                    state = utf8_encode_cp(*sin, value_out, state);
                    result = JLT_UTF_ENCODE_NEXT_STATE(state);
                } while(result && result != 4);

                if(result) {
                    // Error
                    nc = sizeof(UTF8_CP_REPLACEMENT) / sizeof(UTF8_CP_REPLACEMENT[0]);
                }

                tot_bytes_out += nc;
                ++sin;
            }

            return tot_bytes_out;
        }
    } // namespace text
} // namespace jolt
