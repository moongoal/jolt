#include <utility>
#include <test.hpp>
#include <threading/thread.hpp>
#include <collections/vector.hpp>
#include <collections/hashmap.hpp>

using namespace jolt::memory;
using namespace jolt::collections;

struct TestStruct {
    int a = 5;

    TestStruct(int value) : a{value} {}

    bool operator==(const TestStruct &other) const { return a == other.a; }
};

using hash_map = HashMap<int, TestStruct>;

size_t mem_begin;

SETUP {
    jolt::threading::initialize();
    mem_begin = get_allocated_size();
}

TEST(add) {
    hash_map m;

    m.add(1, 10);
    m.add(2, 30);
    m.add(1, 30);

    assert2(*m.get_value(1) == 30, "Duplicate value");
    assert2(*m.get_value(2) == 30, "Non-duplicate value");
    assert2(m.get_value(3) == nullptr, "Non-existent value");
}

TEST(get_length) {
    hash_map m;

    m.add(1, 10);
    m.add(2, 30);
    m.add(1, 30);

    assert(m.get_length() == 2);
}

TEST(ctor__copy) {
    hash_map m;

    m.add(1, 10);
    m.add(2, 30);
    m.add(1, 30);

    hash_map m2{m};

    assert2(*m2.get_value(1) == 30, "Duplicate value");
    assert2(*m2.get_value(2) == 30, "Non-duplicate value");
    assert2(m2.get_value(3) == nullptr, "Non-existent value");
}

TEST(ctor__move) {
    hash_map m;

    m.add(1, 10);
    m.add(2, 30);
    m.add(1, 30);

    hash_map m2{std::move(m)};

    assert2(*m2.get_value(1) == 30, "Duplicate value");
    assert2(*m2.get_value(2) == 30, "Non-duplicate value");
    assert2(m2.get_value(3) == nullptr, "Non-existent value");
}

TEST(contains_key) {
    hash_map m;

    m.add(1, 10);
    m.add(2, 30);
    m.add(1, 30);

    assert2(m.contains_key(1), "Existing key");
    assert2(!m.contains_key(100), "Non-existing key");
}

TEST(get_pair) {
    hash_map m;

    m.add(1, 10);
    m.add(2, 30);
    m.add(1, 30);

    auto pair1 = m.get_pair(2);
    auto pair2 = m.get_pair(3);

    assert2(pair1->get_key() == 2 && pair1->get_value() == 30, "Existing key");
    assert2(!pair2, "Non-existing key");
}

TEST(get_value) {
    hash_map m;

    m.add(1, 10);
    m.add(2, 30);

    assert2(*m.get_value(1) == 10, "Existing key");
    assert2(!m.get_value(3), "Non-existing key");
}

TEST(remove) {
    hash_map m;

    m.add(1, 10);
    m.add(2, 30);

    m.remove(1);

    assert2(m.get_length() == 1, "First removed");

    m.remove(1);
    assert2(m.get_length() == 1, "First removed again");

    m.remove(2);
    assert2(m.get_length() == 0, "Second removed");
}

TEST(clear) {
    hash_map m;

    m.add(1, 10);
    m.add(2, 30);

    m.clear();

    assert(m.get_length() == 0);
}

TEST(memory_leaks) { // Must be last test
    assert(get_allocated_size() == mem_begin);
}
