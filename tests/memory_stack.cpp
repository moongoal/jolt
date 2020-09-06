#include <test.hpp>
#include <memory/stack.hpp>
#include <memory/heap.hpp>
#include <features.hpp>
#include <memory/checks.hpp>

using namespace jolt;
using namespace jolt::memory;

constexpr size_t test_heap_size = (1024 > Heap::MIN_ALLOC_SIZE) ? 1024 : Heap::MIN_ALLOC_SIZE;

TEST(allocate) {
    Stack stack(test_heap_size);

    // Create blocks
    uint8_t *const b1 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    void *const before_alloc = stack.get_top();
    uint8_t *const b2 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    void *const after_alloc = stack.get_top();

    assert(b1);
    assert(b2);
    assert(b2 > b1);

    assert(after_alloc > before_alloc);

    // Fill blocks
    for(int i = 0; i < 256; ++i) { b1[i] = 0xfe; }

    for(int i = 0; i < 256; ++i) { b2[i] = 0xab; }

    // Check block integrity
    for(int i = 0; i < 256; ++i) { assert(b1[i] == 0xfe); }

    for(int i = 0; i < 256; ++i) { assert(b2[i] == 0xab); }

    // Check allocation headers
    StackAllocHeader *const h1 = Stack::get_header(b1);
    StackAllocHeader *const h2 = Stack::get_header(b2);

    // Ensure overflow guards are in place
    JLT_CHECK_OVERFLOW(b1, 256);
    JLT_CHECK_OVERFLOW(b2, 256);

    // Ensure padding is within limits
    assert(h1->m_alloc_offset >= 0 && h1->m_alloc_offset < 16);
    assert(h2->m_alloc_offset >= 0 && h2->m_alloc_offset < 16);

    // Ensure allocation size is correct
    assert(h1->m_alloc_sz == 256);
    assert(h2->m_alloc_sz == 256);
}

TEST(free) {
    Stack stack(test_heap_size);

    uint8_t *const b1 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));

    void *const top_before_alloc = stack.get_top();
    size_t const allocated_before_alloc = stack.get_allocated_size();
    uint8_t *const b2 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    size_t const committed_after_alloc = stack.get_committed_size();

    stack.free(b2);

    void *const top_after_free = stack.get_top();
    size_t const allocated_after_free = stack.get_allocated_size();
    size_t const committed_after_free = stack.get_committed_size();

    assert(top_before_alloc == top_after_free);
    assert(allocated_before_alloc == allocated_after_free);
    assert(committed_after_alloc == committed_after_free);
}

TEST(free2) {
    Stack stack(test_heap_size);

    uint8_t *const b1 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    uint8_t *const b2 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));

    stack.free(b2);
    stack.free(b1);

    assert(stack.get_allocated_size() == 0);
    assert(stack.get_committed_size() > 0);
    assert(stack.get_top() == stack.get_base());
}

TEST(ensure_free_memory_consistency) {
    Stack stack(test_heap_size);

    uint8_t *const b1 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    uint8_t *const b2 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));

    stack.free(b2);
    stack.ensure_free_memory_consistency(b2, 256); // This is going to abort upon failure
}

TEST(reallocate_shrink) {
    Stack stack(test_heap_size);

    uint8_t *const b1 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    uint8_t *const b2 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    StackAllocHeader *const ptr_hdr_b2 = Stack::get_header(b2);

    stack.reallocate(b2, 128);

    assert(ptr_hdr_b2->m_alloc_sz == 128);

    JLT_CHECK_OVERFLOW(b2, 128);
}

TEST(reallocate_grow) {
    Stack stack(test_heap_size * 2);

    uint8_t *const b1 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    uint8_t *const b2 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    StackAllocHeader *const ptr_hdr_b2 = Stack::get_header(b2);

    stack.reallocate(b2, test_heap_size);

    assert(ptr_hdr_b2->m_alloc_sz == test_heap_size);

    JLT_CHECK_OVERFLOW(b2, test_heap_size);
}

TEST(reallocate_nop) {
    Stack stack(test_heap_size);

    uint8_t *const b1 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    uint8_t *const b2 = reinterpret_cast<uint8_t *>(stack.allocate(256, 16));
    StackAllocHeader *const ptr_hdr_b2 = Stack::get_header(b2);

    stack.reallocate(b2, 256);

    assert(ptr_hdr_b2->m_alloc_sz == 256);

    JLT_CHECK_OVERFLOW(b2, 256);
}
