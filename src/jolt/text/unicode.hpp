#ifndef JLT_TEXT_UTF8_HPP
#define JLT_TEXT_UTF8_HPP

#include <cstdint>
#include <jolt/util.hpp>

/**
 * Decode the amount of characters to move forward in a UTF-8 string from the state word as modified
 * by `utf8_decode_cp()`.
 */
#define JLT_UTF_DECODE_STATE_INC_AMOUNT(s) (((s)&0x80) >> 7)

/**
 * Extract the state of the operation from the state word.
 *
 * @param s The state word as returned from utf8_decode_cp().
 *
 * @return The state of the encoding operation. Notable states include:
 * - UTF8_DECODE_STATE_SUCCESS
 * - UTF8_DECODE_STATE_ERROR
 *
 * Any other returned value is to be interpreted as a non-complete encoding. In this case, a
 * further call to the function is required.
 */
#define JLT_UTF_DECODE_NEXT_STATE(s) ((s)&0x7F)

/**
 * Extract the state of the operation from the state word.
 *
 * @param s The state word as returned from utf8_encode_cp().
 *
 * @return The state of the encoding operation. Notable states include:
 * - UTF8_ENCODE_STATE_SUCCESS
 * - UTF8_ENCODE_STATE_ERROR
 *
 * Any other returned value is to be interpreted as a non-complete encoding. In this case, a
 * further call to the function is required.
 */
#define JLT_UTF_ENCODE_NEXT_STATE(s) ((s >> 5) & 0x001f)

namespace jolt {
    namespace text {
        using utf8c = char8_t;
        using utf16c = uint16_t;
        using utf32c = uint32_t;

        constexpr int UNICODE_VERSION_MAJOR = 13; //< Unicode standard major version
        constexpr int UNICODE_VERSION_MINOR = 0;  //< Unicode standard minor version

        constexpr uint16_t UTF8_DECODE_STATE_INIT = 0x8000;
        constexpr uint16_t UTF8_ENCODE_STATE_INIT = 0x0800;
        constexpr uint8_t UTF8_DECODE_STATE_ERROR = 2; //< Decode state indicating an error.
        constexpr uint8_t UTF8_DECODE_STATE_SUCCESS =
          0; //< Decode state indicating success. Only valid when utf8_decode_cp() returns true.
        constexpr uint8_t UTF8_ENCODE_STATE_SUCCESS = 0; //< Encode state indicating an error.
        constexpr uint8_t UTF8_ENCODE_STATE_ERROR = 4;   //< Encode state indicating success.

        constexpr utf32c UNICODE_CP_REPLACEMENT = 0xFFFD;           //< Replacement codepoint.
        constexpr utf8c UTF8_CP_REPLACEMENT[] = {0xEF, 0xBF, 0xBD}; //< Replacement codepoint.
        constexpr utf8c UTF8_SIGNATURE[] = {0xEF, 0xBB, 0xBF};      //< UTF-8 BOM signature

        enum class Encoding { UTF8, UTF16, UTF32 };

        constexpr bool utf_is_non_character(utf32c c) {
            return ((c & 0xFFFF) >= 0xFFFE) || (c >= 0xFD00 && c <= 0xFDEF);
        }

        constexpr bool utf_is_high_surrogate(utf32c c) { return c >= 0xD800 && c <= 0xDBFF; }
        constexpr bool utf_is_low_surrogate(utf32c c) { return c >= 0xDC00 && c <= 0xDFFF; }

        constexpr bool utf_is_scalar(utf32c c) {
            return c <= 0xD7FF || (c >= 0xE000 && c <= 0x10FFFF);
        }

        /**
         * Decode a single UTF-8 code point.
         * This function decodes a single UTF-8 code point into a UTF-32 code point, one scalar
         * value at a time. The intermediate state of each operation is stored into the value
         * pointed at by `state` and the intermediate result is stored in `out`.
         *
         * @param in The input UTF-8 scalar value.
         * @param out Pointer to the output UTF-32 code point.
         * @param state State of the operation.
         *
         * @return The next state word. The JLT_UTF_DECODE_NEXT_STATE() macro can be called upon
         * the returned value and its result checked against UTF8_DECODE_STATE_ERROR or
         * UTF8_DECODE_STATE_SUCCESS to establish whether an ill-formed sequence was encountered.
         * With any other value returned by the macro, further processing is required and the
         * function shall be called again. Before calling the function, the
         * JLT_UTF_DECODE_STATE_INC_AMOUNT() macro should be called to determine whether the
         * function requires to be called with the same UTF-8 scalar (the macro returns 0) or with
         * its next sibling in the sequence (the macro returns 1).
         *
         * @remark Before starting to decode a new code point, the value pointed at by `out` should
         * be set to 0 and the one pointed at by `state` should be set to UTF8_DECODE_STATE_INIT.
         */
        uint16_t JLTAPI utf8_decode_cp(const utf8c in, utf32c *const out, uint16_t const state);

        /**
         * Encode a character to UTF-8.
         *
         * @param in The UTF-32 input character.
         * @param out The UTF-8 output character buffer. The buffer should be of length four if the
         * output size is unknown.
         * @param state The state word. This should be initialized to UTF8_ENCODE_STATE_INIT when
         * starting to encode a character.
         *
         * @return The next state. The return value will need to be passed in as the `state`
         * parameter for the following call if a such a call is required. The returned state can
         * also be inspected by using the JLT_UTF_ENCODE_NEXT_STATE() macro and determine whether
         * further processing is required, the character has been encoded correctly or an error has
         * occurred.
         */
        uint16_t JLTAPI utf8_encode_cp(const utf32c in, utf8c *const out, uint16_t const state);

        /**
         * Encode a UTF-32 string to UTF-8.
         *
         * @param sin The input string.
         * @param sin_len The input string length.
         * @param sout The output buffer.
         * @param sout_len The output buffer length.
         *
         * @return The number of UTF-8 characters in the output buffer filled during the encoding
         * operation. If the return value is the same as `sout_len`, the output could have been
         * truncated.
         */
        size_t JLTAPI
        utf8_encode(const utf32c *sin, size_t const sin_len, utf8c *sout, size_t const sout_len);

        /**
         * Compute the required output buffer length for usage with `utf_encode()`.
         *
         * @param sin The input string.
         * @param sin_len The input string length.
         *
         * @return The length of the UTF-8 string that would be returned by `utf8_encode()` for the
         * same arguments.
         */
        size_t JLTAPI utf8_encode_buffer_len(const utf32c *sin, size_t const sin_len);

        /**
         * Decode an UTF-8 sequence into its UTF-32 equivalent.
         *
         * @param sin The input sequence.
         * @param sin_len The length (number of utf8c items) of the input sequence.
         * @param sout The output sequence.
         * @param sout_len The length of the output sequence.
         *
         * @remark If the input UTF code point sequence is longer than the output buffer,
         * the output is truncated to its length.
         *
         * @remark This function *does not* return a NUL-terminated sequence.
         */
        void JLTAPI
        utf8_decode(const utf8c *sin, size_t const sin_len, utf32c *sout, size_t const sout_len);

        /**
         * Find the beginning of the next code point.
         *
         * @param s The point where to start the search.
         * @param len The number of bytes from `s` to the end of the sequence.
         *
         * @return The pointer to the beginning of the next code point or `nullptr` if there is no
         * next character.
         */
        utf8c *JLTAPI utf8_next_cp(const utf8c *s, long long len);

        /**
         * Find the beginning of the current code point.
         *
         * @param s The point where to start the search.
         * @param s_begin The beginning of the searched string.
         *
         * @return The pointer to the beginning of the current code point or `nullptr` if there is
         * string is Ill-formed.
         */
        utf8c *JLTAPI utf8_cp_start(const utf8c *s, const utf8c *const s_begin);

        /**
         * Compute the length of the input UTF-8 sequence.
         *
         * @param s The sequence to compute the length for.
         * @param s_sz The size in bytes of the sequence.
         */
        size_t JLTAPI utf8_len(const utf8c *s, size_t const s_sz);

        /**
         * Check whether the input UTF-8 sequence is sane or not.
         * A sequence is not sane when it contains any non-meaningful UTF-8 subsequence.
         *
         * @param s The sequence to check for sanity.
         * @param s_sz The size in bytes of the sequence.
         */
        bool JLTAPI utf8_is_sane(const utf8c *s, size_t const s_sz);

    } // namespace text
} // namespace jolt

#endif /* JLT_TEXT_UTF8_HPP */
