#include <utility>
#include <jolt/test.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/collections/linkedlist.hpp>

using namespace jolt::collections;

size_t g_used_memory;

SETUP {
    jolt::threading::initialize();
    g_used_memory = jolt::memory::get_allocated_size();
}

TEST(ctor) {
    LinkedList<int> a{}, b{{1, 2, 3, 4}}, c{std::move(b)}, d{c}, e{d.begin(), d.end()};

    assert2(a.get_length() == 0, "Empty constructor length");
    assert2(b.get_length() == 0, "Moved instance length");
    assert2(c.get_length() == 4, "Move constructor length");
    assert2(d.get_length() == 4, "Copy constructor length");
    assert2(e.get_length() == 4, "Iterator constructor length");

    auto c_end = c.cend();
    auto d_end = d.cend();
    auto e_end = e.cend();
    size_t i = 1;

    for(auto it = c.begin(); it != c_end; ++it) { assert2(*it == i++, "Move constructor value"); }

    i = 1;
    for(auto it = d.begin(); it != d_end; ++it) { assert2(*it == i++, "Copy constructor value"); }

    i = 1;
    for(auto it = e.begin(); it != e_end; ++it) {
        assert2(*it == i++, "Iterator constructor value");
    }

    assert2(a.get_first_node() == nullptr, "Empty constructor first");
    assert2(b.get_first_node() == nullptr, "Moved instance first");
    assert2(c.get_first() == 1, "Move constructor first");
    assert2(d.get_first() == 1, "Copy constructor first");
    assert2(e.get_first() == 1, "Iterator constructor first");

    assert2(a.get_last_node() == nullptr, "Empty constructor last");
    assert2(b.get_last_node() == nullptr, "Moved instance last");
    assert2(c.get_last() == 4, "Move constructor last");
    assert2(d.get_last() == 4, "Copy constructor last");
    assert2(e.get_last() == 4, "Iterator constructor last");
}

TEST(add__add_after) {
    LinkedList<int> a;

    a.add(1);
    assert2(a.get_first() == 1, "First value");

    a.add(2);
    assert2(a.get_first_node()->get_next()->get_value() == 2, "Second value");

    a.add_after(3, a.get_first_node());

    assert2(a.get_first() == 1, "First updated value");
    assert2(a.get_first_node()->get_next()->get_value() == 3, "First updated link");
    assert2(a.get_length() == 3, "Length");
}

TEST(add_all) {
    LinkedList<int> a, b{{1, 2, 3, 4, 5}};

    a.add_all(b.begin(), b.end());

    auto a_end = a.cend();
    auto b_end = b.cend();
    auto a_it = a.begin();
    auto b_it = b.begin();

    for(; a_it != a_end && b_it != b_end; ++a_it, ++b_it) { assert2(*a_it = *b_it, "Value"); }

    assert2(a_it == a_end && b_it == b_end, "Length 1");
    assert2(a.get_length() == 5, "Length 2");
}

TEST(add_all_after) {
    LinkedList<int> a{{0, 6}}, b{{1, 2, 3, 4, 5}};

    a.add_all_after(b.begin(), b.end(), a.get_first_node());

    auto a_end = a.cend();
    auto a_it = a.begin();

    for(size_t i = 0; i <= 6; ++i, ++a_it) { assert(*a_it == i); }

    assert(a_it == a_end);
    assert2(a.get_length() == 7, "Length");
}

TEST(find) {
    LinkedList<int> a{{1, 2, 3, 4, 5}};

    assert(a.find(1) == a.get_first_node());
    assert(a.find(5) == a.get_last_node());
    assert(a.find(2) == a.get_first_node()->get_next());
    assert(a.find(6) == nullptr);
}

TEST(remove) {
    LinkedList<int> a{{1, 2, 3, 4, 5}};

    a.remove(*a.get_first_node());
    assert2(a.get_first() == 2, "First node");

    a.remove(*a.get_last_node());
    assert2(a.get_last() == 4, "Last node");

    auto const node_three = a.find(3);
    a.remove(*node_three);
    assert2(a.find(3) == nullptr, "Middle node");

    a.remove(*a.get_first_node());
    a.remove(*a.get_first_node());

    assert2(a.get_first_node() == nullptr, "First node pointer");
    assert2(a.get_last_node() == nullptr, "Last node pointer");
    assert2(a.get_length() == 0, "Length");
}

TEST(clear) {
    LinkedList<int> a{{1, 2, 3, 4, 5}};

    a.clear();

    assert2(a.get_length() == 0, "Length");
    assert2(a.get_first_node() == nullptr, "First node");
    assert2(a.get_last_node() == nullptr, "Last node");
}

TEST(memory_leaks) { // Must be last test
    assert(g_used_memory == jolt::memory::get_allocated_size());
}
