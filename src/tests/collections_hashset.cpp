#include <utility>
#include <jolt/test.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/collections/vector.hpp>
#include <jolt/collections/hashset.hpp>

using namespace jolt::collections;

struct TestStruct {
    int a = 5;

    TestStruct(int value) : a{value} {}

    bool operator==(const TestStruct &other) const { return a == other.a; }
};

SETUP { jolt::threading::initialize(); }

TEST(ctor) {
    Vector<TestStruct> v{{1, 2, 2, 3, 3, 4, 5}};

    HashSet<TestStruct> s1, s3{{1, 2, 3, 3, 4, 5}}, s4{v.begin(), v.end()};

    assert2(s1.get_length() == 0, "Empty size");
    assert2(s3.get_length() == 5, "Initializer list size");
    assert2(s4.get_length() == s3.get_length(), "Copy size");

    HashSet<TestStruct> s5{std::move(s4)};

    assert2(s5.get_length() == s3.get_length(), "Move size");
}

TEST(add) {
    HashSet<TestStruct> s;

    assert2(s.add(1), "First insertion call");

    assert2(s.get_length() == 1, "First insertion");

    assert2(s.add(2), "Second insertion call");
    assert2(s.get_length() == 2, "Second insertion");

    assert2(!s.add(2), "Duplicate insertion call");
    assert2(s.get_length() == 2, "Duplicate insertion");
}

TEST(add_all) {
    Vector<TestStruct> v1{1, 2, 3, 4, 5, 5};
    Vector<TestStruct> v2{6, 7, 8};

    HashSet<TestStruct> s;

    assert2(!s.add_all(v1.begin(), v1.end()), "With duplicates");
    assert2(s.add_all(v2.begin(), v2.end()), "Without duplicates");
}

TEST(contains) {
    HashSet<TestStruct> s{1, 2, 3};

    assert2(s.contains(1), "Present");
    assert2(!s.contains(4), "Not present");
}

TEST(remove) {
    HashSet<TestStruct> s{1, 2, 3};

    assert2(s.remove(1), "Present");
    assert2(s.get_length() == 2, "Present length");

    assert2(!s.remove(4), "Not present");
    assert2(s.get_length() == 2, "Not present length");
}

TEST(clear) {
    HashSet<TestStruct> s{1, 2, 3};

    s.clear();
    assert(s.get_length() == 0);
}
