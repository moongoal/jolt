#include <jolt/test.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/collections/vector.hpp>

using namespace jolt::memory;
using namespace jolt::collections;

size_t mem_begin;

struct TestStruct {
    int value1 = 0;
    int value2 = 0;

    TestStruct() {}
    ~TestStruct() {}
};

SETUP {
    jolt::threading::initialize();

    mem_begin = get_allocated_size();
}

TEST(ctor) {
    Vector<int> numbers = {1, 2, 3, 4, 5};

    assert2(numbers.get_length() == 5, "Wrong length");

    for(int i = 0; i < 5; ++i) { assert2(numbers[i] == i + 1, "Wrong value"); }
}

TEST(push__pop) {
    Vector<int> numbers;

    for(int i = 0; i < 5; ++i) { numbers.push(i); }
    for(int i = 4; i >= 0; --i) { assert(numbers.pop() == i); }
}

TEST(add_all) {
    Vector<TestStruct> s = {TestStruct{}, TestStruct{}, TestStruct{}};
    TestStruct array[] = {TestStruct{}, TestStruct{}};

    array[1].value1 = 200;
    array[1].value2 = 500;

    assert(s.get_length() == 3);

    s.add_all(array, 2, 2);
    assert(s.get_length() == 5);

    assert(s[3].value1 == 200);
    assert(s[3].value2 == 500);
}

TEST(operator_plus) {
    Vector<int> v1 = {1, 2, 3, 4, 5};
    Vector<int> v2 = {6, 7, 8, 9, 10};
    Vector<int> v3 = v1 + v2;

    for(int i = 1; i <= 10; ++i) { assert(v3[i - 1] == i); }
}

TEST(operator_plus_equal) {
    Vector<int> v = {1, 2, 3, 4, 5};

    v += {6, 7, 8, 9, 10};

    for(int i = 1; i <= 10; ++i) { assert(v[i - 1] == i); }
}

TEST(capacity) {
    Vector<int> v;

    for(int i = 0; i < 10000; ++i) { v.push(i); }

    assert(v.get_length() == 10000);
    assert(v.get_capacity() >= 10000);
}

TEST(find) {
    Vector<int> numbers = {1, 2, 3, 4, 5};

    for(int i = 0; i < 5; ++i) { assert(numbers.find(i + 1) == i); }
}

TEST(remove) {
    Vector<int> numbers = {1, 2, 3, 4, 5};

    numbers.remove(3);

    assert(numbers[0] == 1);
    assert(numbers[1] == 2);
    assert(numbers[2] == 4);
    assert(numbers[3] == 5);
}

TEST(remove_at) {
    Vector<int> numbers = {1, 2, 3, 4, 5};

    numbers.remove_at(2);

    assert(numbers[0] == 1);
    assert(numbers[1] == 2);
    assert(numbers[2] == 4);
    assert(numbers[3] == 5);
}

TEST(clear) {
    Vector<TestStruct> s = {TestStruct{}, TestStruct{}, TestStruct{}};

    s.clear();

    assert(s.get_length() == 0);
}

TEST(memory_leaks) { // Must be last test
    assert(get_allocated_size() == mem_begin);
}
