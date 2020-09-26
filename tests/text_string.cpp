#include <utility>
#include <test.hpp>
#include <memory/allocator.hpp>
#include <threading/thread.hpp>
#include <text/string.hpp>

using namespace jolt::text;

size_t mem_begin;

SETUP {
    jolt::threading::initialize();
    mem_begin = jolt::memory::get_allocated_size();
}

TEST(ctor__utf8c_literal) {
    const utf8c *const s_raw = u8"asd";
    String s{u8"asd"};

    assert2(s.get_length() == 3, "Length not set correctly");

    for(size_t i = 0; i < 3; ++i) { assert2(s[i] == s_raw[i], "Value not set correctly"); }
}

TEST(ctor__char_ptr) {
    const utf8c *const s_raw = u8"asd";
    String s{s_raw, 3};

    assert2(s.get_length() == 3, "Length not set correctly");

    for(size_t i = 0; i < 3; ++i) { assert2(s[i] == s_raw[i], "Value not set correctly"); }
}

TEST(ctor__string_ref_non_literal) {
    String s1{u8"blah blah"};
    String s4 = u8"hey ";
    String s3 = s4 + s1;

    String s2{s3};

    assert2(s2.get_length() == s3.get_length(), "Length not set correctly");
    assert2(s2.get_raw() != s1.get_raw(), "String not cloned");

    for(size_t i = 0; i < 3; ++i) { assert2(s2[i] == s3[i], "Value not set correctly"); }
}

TEST(ctor__string_ref_literal) {
    String s1{u8"blah blah"};
    String s2{s1};

    assert2(s2.get_length() == s1.get_length(), "Length not set correctly");
    assert2(s2.get_raw() == s1.get_raw(), "String not cloned");

    for(size_t i = 0; i < 3; ++i) { assert2(s2[i] == s1[i], "Value not set correctly"); }
}

TEST(ctor__string_rvalue) {
    String s1{u8"blah blah"};
    String s2{std::move(s1)};

    assert2(s2.get_length() == s1.get_length(), "Length not set correctly");
    assert2(s2.get_raw() == s1.get_raw(), "String not cloned");

    for(size_t i = 0; i < 3; ++i) { assert2(s2[i] == s1[i], "Value not set correctly"); }
}

TEST(op_equals) {
    String s1{u8"blah blah"};
    String s2{u8"blah blah"};
    String s3{u8""};
    String s4{u8"xxxx xxxx"};

    assert(s1 == s2);
    assert(s1 != s3);
    assert(s2 != s4);
}

TEST(op_plus) {
    String s1{u8"blah blah"};
    String s2{u8"blah grab"};
    String s3{u8""};
    String s4 = s1 + s2;
    String s5 = s2 + s3;

    for(size_t i = 0; i < s1.get_length(); ++i) { assert(s4[i] == s1[i]); }
    for(size_t i = 0; i < s2.get_length(); ++i) { assert(s4[i + s1.get_length()] == s2[i]); }

    for(size_t i = 0; i < s2.get_length(); ++i) { assert(s5[i] == s2[i]); }
    for(size_t i = 0; i < s3.get_length(); ++i) { assert(s5[i + s2.get_length()] == s3[i]); }
}

TEST(join) {
    String s1{u8"String 1"};
    String s2{u8"String 2"};
    String s3{u8"String 3"};
    String s4 = String::join(s1, s2, s3);
    String expected_out = u8"String 1String 2String 3";

    assert(s4 == expected_out);
}

TEST(memory_leaks) { assert(jolt::memory::get_allocated_size() == mem_begin); }
