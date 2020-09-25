#include <utility>
#include <test.hpp>
#include <threading/thread.hpp>
#include <collections/vector.hpp>
#include <collections/valueset.hpp>

using namespace jolt::collections;

SETUP { jolt::threading::initialize(); }

TEST(ctor) {
    Vector v{{1, 2, 2, 3, 3, 4, 5}};

    ValueSet<int> s1, s3{{1, 2, 3, 3, 4, 5}}, s4{v.begin(), v.end()};

    assert2(s1.get_length() == 0, "Empty size");
    assert2(s3.get_length() == 5, "Initializer list size");
    assert2(s4.get_length() == s3.get_length(), "Copy size");

    ValueSet<int> s5{std::move(s4)};

    assert2(s5.get_length() == s3.get_length(), "Move size");
}

TEST(add) {
    ValueSet<int> s;

    assert2(s.add(1), "First insertion call");

    assert2(s.get_length() == 1, "First insertion");

    assert2(s.add(2), "Second insertion call");
    assert2(s.get_length() == 2, "Second insertion");

    assert2(!s.add(2), "Duplicate insertion call");
    assert2(s.get_length() == 2, "Duplicate insertion");
}

TEST(add_all) {
    Vector<int> v1{1, 2, 3, 4, 5, 5};
    Vector<int> v2{6, 7, 8};

    ValueSet<int> s;

    assert2(!s.add_all(v1.begin(), v1.end()), "With duplicates");
    assert2(s.add_all(v2.begin(), v2.end()), "Without duplicates");
}

TEST(contains) {
    ValueSet<int> s{1, 2, 3};

    assert2(s.contains(1), "Present");
    assert2(!s.contains(4), "Not present");
}

TEST(remove) {
    ValueSet<int> s{1, 2, 3};

    assert2(s.remove(1), "Present");
    assert2(s.get_length() == 2, "Present length");

    assert2(!s.remove(4), "Not present");
    assert2(s.get_length() == 2, "Not present length");
}

TEST(clear) {
    ValueSet<int> s{1, 2, 3};

    s.clear();
    assert(s.get_length() == 0);
}
