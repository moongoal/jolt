#include <utility>
#include <jolt/test.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/text/string.hpp>

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

TEST(merge) {
    String s1{u8"String 1"};
    String s2{u8"String 2"};
    String s3{u8"String 3"};
    String s4 = String::merge(s1, s2, s3);
    String expected_out = u8"String 1String 2String 3";

    assert(s4 == expected_out);
}

TEST(join) {
    String s1{u8"String 1"};
    String s2{u8"String 2"};
    String s3{u8"String 3"};
    String s4 = String::join(", ", s1, s2, s3);
    String expected_out = u8"String 1, String 2, String 3";

    assert(s4 == expected_out);
}

TEST(starts_with) {
    String s{u8"blah blah 8"};
    String ss1{u8"blah"};
    String ss2{u8"blah klah"};
    String ss3{u8"blah blah 8"};

    assert2(s.starts_with(ss1), "Should start with this");
    assert2(!s.starts_with(ss2), "Should not start with this");
    assert2(s.starts_with(ss3), "Should start with this 2");
    assert2(s.starts_with(EmptyString), "Should start with the empty string");
    assert2(!EmptyString.starts_with(s), "Empty string should not start with anything but itself");
}

TEST(ends_with) {
    String s{u8"blah blah 8"};
    String ss1{u8"blah 8"};
    String ss2{u8"klah 8"};
    String ss3{u8"blah blah 8"};

    assert2(s.ends_with(ss1), "Should end with this");
    assert2(!s.ends_with(ss2), "Should not end with this");
    assert2(s.ends_with(ss3), "Should end with this 2");
    assert2(s.ends_with(EmptyString), "Should end with the empty string");
    assert2(!EmptyString.ends_with(s), "Empty string should not end with anything but itself");
}

TEST(find) {
    String s{u8"blah blah 8"};

    assert2(s.find("blah") == 0, "0");
    assert2(s.find("blah", 1) == 5, "1");
    assert2(s.find("bleh") == -1, "2");
    assert2(s.find(s) == 0, "3");
    assert2(s.find(EmptyString) == 0, "4");
    assert2(EmptyString.find("bleh") == -1, "5");
    assert2(s.find(EmptyString, s.get_length() - 1) == s.get_length() - 1, "6");
    assert2(s.find(EmptyString, s.get_length()) == -1, "7");
}

TEST(op_subscript) {
    String s{u8"blah blah 8"};

    assert(s[0] == 'b');
    assert(s[1] == 'l');
    assert(s[s.get_length() - 1] == '8');
}

TEST(replace) {
    String s{u8"blah blah 8"};

    assert2(s.replace("blah", "klah") == "klah blah 8", "Basic");
    assert2(s.replace("klah", "blah") == "blah blah 8", "Missing");
    assert2(s.replace(s, "blah") == "blah", "Full");
    assert2(s.replace("", "blah") == s, "Empty");
    assert2(s.replace(" 8", "") == "blah blah", "Remover");
}

TEST(replace_all) {
    String s{u8"blah blah 8"};

    assert2(s.replace_all("blah", "klah") == "klah klah 8", "Basic");
    assert2(s.replace_all("klah", "blah") == "blah blah 8", "Missing");
    assert2(s.replace_all(s, "blah") == "blah", "Full");
    assert2(s.replace_all("", "blah") == s, "Empty");
    assert2(s.replace_all(" 8", "") == "blah blah", "Remover");
}

TEST(slice) {
    String s{u8"blah blah 8"};

    assert2(s.slice(0, s.get_length()) == s, "Full");
    assert2(s.slice(0, 0) == EmptyString, "Empty");
    assert2(s.slice(0, 4) == "blah", "Beginning");
    assert2(s.slice(5, 4) == "blah", "Middle");
    assert2(s.slice(10, 1) == "8", "End");
    assert2(s.slice(5) == "blah 8", "Middle to end");
}

TEST(memory_leaks) { assert(jolt::memory::get_allocated_size() == mem_begin); }
