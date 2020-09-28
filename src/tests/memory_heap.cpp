#include <jolt/test.hpp>
#include <jolt/memory/heap.hpp>

using namespace jolt;
using namespace jolt::memory;

constexpr size_t test_heap_size = (1024 > Heap::MIN_ALLOC_SIZE) ? 1024 : Heap::MIN_ALLOC_SIZE;

struct HeapExtendTest : Heap {
    HeapExtendTest(size_t const sz) : Heap(sz) {}

    void *redirect_extend(size_t const sz) { return commit(sz); }
};

TEST(ctor) {
    Heap heap(test_heap_size);

    assert(heap.get_base() != 0);
    assert(heap.get_size() == test_heap_size);
    assert(heap.get_committed_size() == 0);
}

TEST(commit) {
    HeapExtendTest heap(test_heap_size);

    assert(heap.get_committed_size() == 0);
    assert(heap.redirect_extend(test_heap_size));
    assert(heap.get_committed_size() == test_heap_size);
}

TEST(extend2) {
    constexpr size_t heap_size = Heap::MIN_ALLOC_SIZE * 2;
    HeapExtendTest heap(heap_size);

    assert(heap.get_committed_size() == 0);
    assert(heap.redirect_extend(Heap::MIN_ALLOC_SIZE));
    assert(heap.get_committed_size() == Heap::MIN_ALLOC_SIZE);
    assert(heap.get_size() == heap_size);
}
