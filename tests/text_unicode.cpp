#include <test.hpp>
#include <text/unicode.hpp>

using namespace jolt;
using namespace jolt::text;

TEST(utf8_decode) {
    const utf8c sin[] = {0x4D, 0xD0, 0xB0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x8C, 0x82};
    utf32c sout[4];
    const utf32c expected_sout[] = {0x4d, 0x430, 0x4e8c, 0x10302};
    size_t const sin_len = sizeof(sin) / sizeof(utf8c);
    size_t const sout_len = sizeof(sout) / sizeof(utf32c);

    utf8_decode(sin, sin_len, sout, sout_len);

    for(size_t i = 0; i < sout_len; ++i) {
        assert2(sout[i] == expected_sout[i], "Output value doesn't match expected.");
    }
}

TEST(utf8_decode__invalid_characters) {
    const utf8c sin[] = {0x4D, 0xD0, 0xB0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x00, 0x82};
    utf32c sout[4];
    const utf32c expected_sout[] = {0x4d, 0x430, 0x4e8c, UNICODE_CP_REPLACEMENT};
    size_t const sin_len = sizeof(sin) / sizeof(utf8c);
    size_t const sout_len = sizeof(sout) / sizeof(utf32c);

    utf8_decode(sin, sin_len, sout, sout_len);

    for(size_t i = 0; i < sout_len; ++i) {
        assert2(sout[i] == expected_sout[i], "Output value doesn't match expected.");
    }
}

TEST(utf8_decode__missing) {
    const utf8c sin[] = {0x4D, 0xD0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x8C, 0x82};
    utf32c sout[4];
    // const utf32c expected_sout[] = {0x4d, UNICODE_CP_REPLACEMENT, 0x10302, 0};
    const utf32c expected_sout[] = {0x4d, UNICODE_CP_REPLACEMENT, 0x4e8c, 0x10302};
    size_t const sin_len = sizeof(sin) / sizeof(utf8c);
    size_t const sout_len = sizeof(sout) / sizeof(utf32c);

    utf8_decode(sin, sin_len, sout, sout_len);

    for(size_t i = 0; i < sout_len; ++i) {
        assert2(sout[i] == expected_sout[i], "Output value doesn't match expected.");
    }
}

TEST(utf8_next_cp) {
    const utf8c sin[] = {0x4D, 0xD0, 0xB0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x8C, 0x82};
    size_t const sin_len = sizeof(sin) / sizeof(utf8c);

    assert(utf8_next_cp(sin, sin_len) == &sin[1]); // One byte

    assert(utf8_next_cp(&sin[1], sin_len - 1) == &sin[3]); // Two bytes
    assert(utf8_next_cp(&sin[2], sin_len - 2) == &sin[3]);

    assert(utf8_next_cp(&sin[3], sin_len - 3) == &sin[6]); // Three bytes
    assert(utf8_next_cp(&sin[4], sin_len - 4) == &sin[6]);
    assert(utf8_next_cp(&sin[5], sin_len - 5) == &sin[6]);

    assert(!utf8_next_cp(&sin[6], sin_len - 6)); // Four bytes
    assert(!utf8_next_cp(&sin[7], sin_len - 7));
    assert(!utf8_next_cp(&sin[8], sin_len - 8));
    assert(!utf8_next_cp(&sin[9], sin_len - 9));
}

TEST(utf8_len) {
    const utf8c sin[] = {0x4D, 0xD0, 0xB0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x8C, 0x82};
    size_t const sin_sz = sizeof(sin) / sizeof(utf8c);

    assert(utf8_len(sin, sin_sz) == 4);
}

TEST(utf8_len__1) {
    const utf8c sin[] = {0xF0, 0x90, 0x8C, 0x82};
    size_t const sin_sz = sizeof(sin) / sizeof(utf8c);

    assert(utf8_len(sin, sin_sz) == 1);
}

TEST(utf8_len__0) {
    const utf8c sin[] = {};
    size_t const sin_sz = sizeof(sin) / sizeof(utf8c);

    assert(utf8_len(sin, sin_sz) == 0);
}

TEST(utf8_is_sane__0) {
    const utf8c sin[] = {};
    size_t const sin_sz = sizeof(sin) / sizeof(utf8c);

    assert(utf8_is_sane(sin, sin_sz));
}

TEST(utf8_is_sane__1) {
    const utf8c sin[] = {0x4D};
    size_t const sin_sz = sizeof(sin) / sizeof(utf8c);

    assert(utf8_is_sane(sin, sin_sz));
}

TEST(utf8_is_sane) {
    const utf8c sin[] = {0x4D, 0xD0, 0xB0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x8C, 0x82};
    size_t const sin_sz = sizeof(sin) / sizeof(utf8c);

    assert(utf8_is_sane(sin, sin_sz));
}

TEST(utf8_is_sane__invalid_characters) {
    const utf8c sin[] = {0x4D, 0xD0, 0xB0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x00, 0x82};
    size_t const sin_len = sizeof(sin) / sizeof(utf8c);

    assert(!utf8_is_sane(sin, sin_len));
}

TEST(utf8_is_sane__missing) {
    const utf8c sin[] = {0x4D, 0xD0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x8C, 0x82};
    size_t const sin_len = sizeof(sin) / sizeof(utf8c);

    assert(!utf8_is_sane(sin, sin_len));
}

TEST(utf8_encode) {
    const utf32c sin[] = {0x4d, 0x430, 0x4e8c, 0x10302};
    utf8c sout[sizeof(sin) * 4];
    const utf8c expected_sout[] = {0x4D, 0xD0, 0xB0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x8C, 0x82};
    size_t const sin_len = sizeof(sin) / sizeof(utf32c);
    size_t const sout_len = sizeof(sout) / sizeof(utf8c);
    size_t const expected_sout_len = sizeof(expected_sout) / sizeof(utf8c);

    size_t const out_bytes = utf8_encode(sin, sin_len, sout, sout_len);

    assert2(expected_sout_len == out_bytes, "Output length not as expected.");

    for(size_t i = 0; i < out_bytes; ++i) {
        assert2(sout[i] == expected_sout[i], "Output value doesn't match expected.");
    }
}

TEST(utf8_encode__invalid_characters) {
    const utf32c sin[] = {0x4d, 0x430, 0x4e8c, 0xfff10302};
    utf8c sout[sizeof(sin) * 4];
    const utf8c expected_sout[] = {
      0x4D,
      0xD0,
      0xB0,
      0xE4,
      0xBA,
      0x8C,
      UTF8_CP_REPLACEMENT[0],
      UTF8_CP_REPLACEMENT[1],
      UTF8_CP_REPLACEMENT[2]};
    size_t const sin_len = sizeof(sin) / sizeof(utf32c);
    size_t const sout_len = sizeof(sout) / sizeof(utf8c);
    size_t const expected_sout_len = sizeof(expected_sout) / sizeof(utf8c);

    size_t const out_bytes = utf8_encode(sin, sin_len, sout, sout_len);

    assert2(expected_sout_len == out_bytes, "Output length not as expected.");

    for(size_t i = 0; i < out_bytes; ++i) {
        assert2(sout[i] == expected_sout[i], "Output value doesn't match expected.");
    }
}

TEST(utf8_encode_buffer_len) {
    const utf32c sin[] = {0x4d, 0x430, 0x4e8c, 0x10302};
    size_t const sin_len = sizeof(sin) / sizeof(utf32c);
    size_t const expected_sout_len = 10;

    size_t const computed_len = utf8_encode_buffer_len(sin, sin_len);

    assert2(expected_sout_len == computed_len, "Output length not as expected.");
}

TEST(utf8_encode_buffer_len__0) {
    const utf32c sin[] = {};
    size_t const sin_len = sizeof(sin) / sizeof(utf32c);
    size_t const expected_sout_len = 0;

    size_t const computed_len = utf8_encode_buffer_len(sin, sin_len);

    assert2(expected_sout_len == computed_len, "Output length not as expected.");
}

TEST(utf8_encode_buffer_len__invalid_character) {
    const utf32c sin[] = {0x4d, 0x430, 0x4e8c, 0xfff10302};
    size_t const sin_len = sizeof(sin) / sizeof(utf32c);
    size_t const expected_sout_len = 9;

    size_t const computed_len = utf8_encode_buffer_len(sin, sin_len);

    assert2(expected_sout_len == computed_len, "Output length not as expected.");
}

TEST(utf8_cp_start) {
    const utf8c sin[] = {0x4D, 0xD0, 0xB0, 0xE4, 0xBA, 0x8C, 0xF0, 0x90, 0x8C, 0x82};
    size_t const sin_len = sizeof(sin) / sizeof(utf8c);

    assert(utf8_cp_start(sin, sin) == sin); // One byte

    assert(utf8_cp_start(sin + 1, sin) == sin + 1); // Two bytes
    assert(utf8_cp_start(sin + 2, sin) == sin + 1);

    assert(utf8_cp_start(sin + 3, sin) == sin + 3); // Three bytes
    assert(utf8_cp_start(sin + 4, sin) == sin + 3);
    assert(utf8_cp_start(sin + 5, sin) == sin + 3);

    assert(utf8_cp_start(sin + 6, sin) == sin + 6); // Four bytes
    assert(utf8_cp_start(sin + 7, sin) == sin + 6);
    assert(utf8_cp_start(sin + 8, sin) == sin + 6);
    assert(utf8_cp_start(sin + 9, sin) == sin + 6);
}
