#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

#include <cstring>
#include <jolt/test.hpp>
#include <jolt/memory/arena.hpp>

using namespace jolt;
using namespace jolt::memory;

#define DEFAULT_HEAP_SIZE (1024 * 1024)
constexpr size_t test_heap_size =
  (DEFAULT_HEAP_SIZE > Heap::MIN_ALLOC_SIZE) ? DEFAULT_HEAP_SIZE : Heap::MIN_ALLOC_SIZE;

TEST(ctor) {
    Arena arena(test_heap_size);
    auto const free_list = reinterpret_cast<ArenaFreeListNode *>(arena.get_base());

    assert(!free_list->m_next);
    assert(!free_list->m_prev);
    assert(free_list->m_size == test_heap_size);
}

TEST(allocate1) { // Full allocation
    constexpr size_t alloc_size = Heap::MIN_ALLOC_SIZE - sizeof(AllocHeader) - JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE;
    Arena arena(Heap::MIN_ALLOC_SIZE);

    void *const b1 = arena.allocate(alloc_size, ALLOC_NONE, 1);
    auto const h1 = reinterpret_cast<AllocHeader *>(b1) - 1;

    assert(
      h1->m_alloc_offset == reinterpret_cast<uint8_t *>(h1) - reinterpret_cast<uint8_t *>(arena.get_base()));
    assert(h1->m_alloc_sz == alloc_size);
    assert2(!arena.get_free_list(), "Free list has to be null");
    JLT_CHECK_OVERFLOW(b1, h1->m_alloc_sz);
}

TEST(allocate2) {
    // Test allocation of three blocks, nothing has to be overridden
    Arena arena(test_heap_size);
    ArenaFreeListNode *const free_list = arena.get_free_list();

    void *const b1 = arena.allocate(1024, ALLOC_NONE, 16);
    auto const h1 = reinterpret_cast<AllocHeader *>(b1) - 1;

    void *const b2 = arena.allocate(256, ALLOC_NONE, 8);
    auto const h2 = reinterpret_cast<AllocHeader *>(b2) - 1;

    void *const b3 = arena.allocate(5, ALLOC_NONE, 64);
    auto const h3 = reinterpret_cast<AllocHeader *>(b3) - 1;

    auto const new_free_list = reinterpret_cast<ArenaFreeListNode *>(
      reinterpret_cast<uint8_t *>(b3) + h3->m_alloc_sz + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE);

    assert(align_raw_ptr(b1, 16) == b1);
    assert(
      h1->m_alloc_offset == reinterpret_cast<uint8_t *>(h1) - reinterpret_cast<uint8_t *>(arena.get_base()));
    assert(h1->m_alloc_sz == 1024);

    assert(align_raw_ptr(b2, 8) == b2);
    assert(
      h2->m_alloc_offset
      == reinterpret_cast<uint8_t *>(h2) - reinterpret_cast<uint8_t *>(b1) - h1->m_alloc_sz
           - JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE);
    assert(h2->m_alloc_sz == 256);

    assert(align_raw_ptr(b3, 64) == b3);
    assert(
      h3->m_alloc_offset
      == reinterpret_cast<uint8_t *>(h3) - reinterpret_cast<uint8_t *>(b2) - h2->m_alloc_sz
           - JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE);
    assert(h3->m_alloc_sz == 5);

    assert(new_free_list != free_list);
    assert(new_free_list == arena.get_free_list());
    assert(
      new_free_list->m_size
      == arena.get_size() - 3 * sizeof(AllocHeader) - h1->m_alloc_offset - h1->m_alloc_sz - h2->m_alloc_offset
           - h2->m_alloc_sz - h3->m_alloc_offset - h3->m_alloc_sz - 3 * JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE);
}

TEST(overflow_canary_location) { // Issue #14
    constexpr size_t alloc_size = 9;
    Arena arena(Heap::MIN_ALLOC_SIZE * 2);

    void *const x = arena.allocate(24, ALLOC_NONE, 1);
    arena.allocate(24, ALLOC_NONE, 1);
    arena.free(x);
    void *const b1 = arena.allocate(alloc_size, ALLOC_NONE, 1);
    auto const h1 = reinterpret_cast<AllocHeader *>(b1) - 1;

    JLT_CHECK_OVERFLOW(b1, h1->m_alloc_sz);
}

TEST(free__right_free_node_available) { // Single free (only right free list node available)
    Arena arena(test_heap_size);
    ArenaFreeListNode *const free_list = arena.get_free_list();
    size_t const available_memory = free_list->m_size;
    void *const alloc = arena.allocate(128, ALLOC_NONE, 1);

    arena.free(alloc);
    arena.ensure_free_memory_consistency(free_list);

    assert2(arena.get_free_list() == free_list, "Free list after free should be the same as initial value.");
    assert2(
      free_list->m_size == available_memory,
      "Available memory after free should be the same as initial value.");
    assert2(
      free_list->m_next == free_list->m_prev && !free_list->m_prev,
      "Next and previous pointer should be null");
}

TEST(free__no_free_node_available) { // Single free with full memory (no free list node available)
    constexpr size_t alloc_size = Heap::MIN_ALLOC_SIZE - sizeof(AllocHeader) - JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE;
    Arena arena(Heap::MIN_ALLOC_SIZE);
    ArenaFreeListNode *const free_list = arena.get_free_list();
    size_t const available_memory = free_list->m_size;
    void *const alloc = arena.allocate(alloc_size, ALLOC_NONE, 1);

    arena.free(alloc);
    arena.ensure_free_memory_consistency(free_list);

    assert2(arena.get_free_list() == free_list, "Free list after free should be the same as initial value.");
    assert2(
      free_list->m_size == available_memory,
      "Available memory after free should be the same as initial value.");
    assert2(
      free_list->m_next == free_list->m_prev && !free_list->m_prev,
      "Next and previous pointer should be null");
}

TEST(free__left_free_node_available) { // Free with only left free list node available
    Arena arena(test_heap_size);
    ArenaFreeListNode *const free_list = arena.get_free_list();
    size_t const available_memory = free_list->m_size;
    void *const alloc1 = arena.allocate(128, ALLOC_NONE, 1);
    void *const alloc2 = arena.allocate(128, ALLOC_NONE, 1);

    arena.free(alloc1);
    arena.free(alloc2); // That's the interesting line for the test
    arena.ensure_free_memory_consistency(free_list);

    assert2(arena.get_free_list() == free_list, "Free list after free should be the same as initial value.");
    assert2(
      free_list->m_size == available_memory,
      "Available memory after free should be the same as initial value.");
    assert2(
      free_list->m_next == free_list->m_prev && !free_list->m_prev,
      "Next and previous pointer should be null");
}

TEST(free__both_free_nodes_available) { // Free with both left & right free list nodes available
    Arena arena(test_heap_size);
    ArenaFreeListNode *const free_list = arena.get_free_list();
    size_t const available_memory = free_list->m_size;
    void *const alloc1 = arena.allocate(128, ALLOC_NONE, 1);
    void *const alloc2 = arena.allocate(128, ALLOC_NONE, 1);
    void *const alloc3 = arena.allocate(128, ALLOC_NONE, 1);

    arena.free(alloc1);
    arena.free(alloc3);
    arena.free(alloc2); // That's the interesting line for the test
    arena.ensure_free_memory_consistency(free_list);

    assert2(arena.get_free_list() == free_list, "Free list after free should be the same as initial value.");
    assert2(
      free_list->m_size == available_memory,
      "Available memory after free should be the same as initial value.");
    assert2(
      free_list->m_next == free_list->m_prev && !free_list->m_prev,
      "Next and previous pointer should be null");
}

TEST(reallocate__shrink__nop) { // Full allocation
    constexpr size_t alloc_size = Heap::MIN_ALLOC_SIZE - sizeof(AllocHeader) - JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE;
    Arena arena(Heap::MIN_ALLOC_SIZE);

    void *const b1 = arena.allocate(alloc_size, ALLOC_NONE, 1);
    auto const h1 = Arena::get_header(b1);
    void *const b1_realloc = arena.reallocate(b1, alloc_size);

    assert(b1_realloc == b1);
    assert(
      h1->m_alloc_offset == reinterpret_cast<uint8_t *>(h1) - reinterpret_cast<uint8_t *>(arena.get_base()));
    assert(h1->m_alloc_sz == alloc_size);
    assert2(!arena.get_free_list(), "Free list has to be null");
    JLT_CHECK_OVERFLOW(b1_realloc, h1->m_alloc_sz);
}

TEST(reallocate__shrink__no_change) { // Full allocation
    constexpr size_t alloc_size = Heap::MIN_ALLOC_SIZE - sizeof(AllocHeader) - JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE;
    Arena arena(Heap::MIN_ALLOC_SIZE);

    void *const b1 = arena.allocate(alloc_size, ALLOC_NONE, 1);
    auto const h1 = Arena::get_header(b1);
    void *const b1_realloc = arena.reallocate(b1, alloc_size - 1);

    assert(b1_realloc == b1);
    assert(
      h1->m_alloc_offset == reinterpret_cast<uint8_t *>(h1) - reinterpret_cast<uint8_t *>(arena.get_base()));
    assert(h1->m_alloc_sz == alloc_size);
    assert2(!arena.get_free_list(), "Free list has to be null");
    JLT_CHECK_OVERFLOW(b1_realloc, h1->m_alloc_sz);
}

TEST(reallocate__shrink__with_change) { // Full allocation
    constexpr size_t alloc_size = Heap::MIN_ALLOC_SIZE - sizeof(AllocHeader) - JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE;
    Arena arena(Heap::MIN_ALLOC_SIZE);

    void *const b1 = arena.allocate(alloc_size, ALLOC_NONE, 1);
    auto const h1 = Arena::get_header(b1);
    JLT_MAYBE_UNUSED size_t const b1_sz = h1->m_alloc_sz;
    void *const b1_realloc = arena.reallocate(b1, alloc_size - sizeof(ArenaFreeListNode));
    ArenaFreeListNode *const free_list = arena.get_free_list();

    assert(b1_realloc == b1);
    assert(
      h1->m_alloc_offset == reinterpret_cast<uint8_t *>(h1) - reinterpret_cast<uint8_t *>(arena.get_base()));
    assert2(h1->m_alloc_sz == alloc_size - sizeof(ArenaFreeListNode), "Reallocated size is not correct");
    assert2(free_list, "Free list has not to be null");
    assert2(free_list->m_size == sizeof(ArenaFreeListNode), "Free size not correct");
    JLT_CHECK_OVERFLOW(b1_realloc, h1->m_alloc_sz);
}

TEST(reallocate__grow_with_move) {
    // Test allocation of three blocks, nothing has to be overridden
    Arena arena(test_heap_size);
    ArenaFreeListNode *const free_list = arena.get_free_list();

    void *const b1 = arena.allocate(1024, ALLOC_NONE, 16);
    auto const h1 = Arena::get_header(b1);

    void *const b2 = arena.allocate(256, ALLOC_NONE, 8);
    auto const h2 = Arena::get_header(b2);
    void *const b2_raw_ptr = reinterpret_cast<uint8_t *>(b2) - h2->m_alloc_offset - sizeof(AllocHeader);

    void *const b3 = arena.allocate(5, ALLOC_NONE, 64);
    auto const h3 = Arena::get_header(b3);

    void *const b2_realloc = arena.reallocate(b2, 257);
    auto const h2_realloc = Arena::get_header(b2_realloc);

    auto const new_free_list = reinterpret_cast<ArenaFreeListNode *>(
      reinterpret_cast<uint8_t *>(b2_realloc) + h2_realloc->m_alloc_sz + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE);

    assert(align_raw_ptr(b1, 16) == b1);
    assert(
      h1->m_alloc_offset == reinterpret_cast<uint8_t *>(h1) - reinterpret_cast<uint8_t *>(arena.get_base()));
    assert(h1->m_alloc_sz == 1024);

    assert(align_raw_ptr(b2_realloc, 8) == b2_realloc);
    assert(h2_realloc->m_alloc_sz == 257);
    assert(align_raw_ptr(b3, 64) == b3);
    assert(h3->m_alloc_sz == 5);
    assert(new_free_list != free_list);
    assert(b2_raw_ptr == arena.get_free_list());
    assert(new_free_list->m_prev == b2_raw_ptr);
}

TEST(reallocate__grow_no_move) {
    // Test allocation of three blocks, nothing has to be overridden
    Arena arena(test_heap_size);
    ArenaFreeListNode *const free_list = arena.get_free_list();

    void *const b1 = arena.allocate(1024, ALLOC_NONE, 16);
    auto const h1 = Arena::get_header(b1);

    JLT_MAYBE_UNUSED void *const b2 = arena.allocate(256, ALLOC_NONE, 8);

    void *const b3 = arena.allocate(5, ALLOC_NONE, 64);
    auto const h3 = Arena::get_header(b3);

    void *const b3_realloc = arena.reallocate(b3, 256);
    auto const h3_realloc = Arena::get_header(b3_realloc);

    auto const new_free_list = reinterpret_cast<ArenaFreeListNode *>(
      reinterpret_cast<uint8_t *>(b3_realloc) + h3_realloc->m_alloc_sz + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE);

    assert(align_raw_ptr(b1, 16) == b1);
    assert(
      h1->m_alloc_offset == reinterpret_cast<uint8_t *>(h1) - reinterpret_cast<uint8_t *>(arena.get_base()));
    assert(h1->m_alloc_sz == 1024);

    assert(align_raw_ptr(b3_realloc, 64) == b3_realloc);
    assert(h3_realloc == h3);
    assert(h3_realloc->m_alloc_sz == 256);
    assert(new_free_list != free_list);
    assert(new_free_list == arena.get_free_list());
    assert(!new_free_list->m_prev);
    assert(!new_free_list->m_next);
}

TEST(fill_after_free) {
    Arena arena(test_heap_size);

    void *const a = arena.allocate(sizeof(int) * 500, ALLOC_NONE, alignof(int));
    arena.free(a); // This will crash if fill goes beyond committed memory
}

TEST(will_relocate) {
    Arena arena(test_heap_size);

    void *const b1 = arena.allocate(1024, ALLOC_NONE, 16);
    void *const b2 = arena.allocate(256, ALLOC_NONE, 8);

    assert(arena.will_relocate(b1, 100'000));
    assert(!arena.will_relocate(b2, 100'000));
}
