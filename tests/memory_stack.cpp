#include <test.hpp>
#include <memory/stack.hpp>
#include <memory/heap.hpp>

using namespace jolt;
using namespace jolt::memory;

constexpr size_t test_heap_size = (1024 > Heap::MIN_ALLOC_SIZE) ? 1024 : Heap::MIN_ALLOC_SIZE;

TEST(allocate) {
    Stack stack(test_heap_size);

    // Create blocks
    unsigned char * const b1 = reinterpret_cast<unsigned char *>(stack.allocate(256, ALLOC_NONE, 16));
    unsigned char * const b2 = reinterpret_cast<unsigned char *>(stack.allocate(256, ALLOC_NONE, 16));

    assert(b1);
    assert(b2);
    assert(b2 > b1);

    // Fill blocks
    for(int i = 0; i < 256; ++i) {
        b1[i] = 0xfe;
    }

    for(int i = 0; i < 256; ++i) {
        b2[i] = 0xab;
    }

    // Check block integrity
    for(int i = 0; i < 256; ++i) {
        assert(b1[i] == 0xfe);
    }

    for(int i = 0; i < 256; ++i) {
        assert(b2[i] == 0xab);
    }

    // Check allocation headers
    StackAllocHeader * const h1 = reinterpret_cast<StackAllocHeader*>(b1 - sizeof(StackAllocHeader));
    StackAllocHeader * const h2 = reinterpret_cast<StackAllocHeader*>(b2 - sizeof(StackAllocHeader));

    // Ensure padding is within limits
    assert(h1->m_alloc_offset >= 0 && h1->m_alloc_offset < 16);
    assert(h2->m_alloc_offset >= 0 && h2->m_alloc_offset < 16);

    // Ensure allocation size is correct
    assert(h1->m_alloc_sz == 256);
    assert(h2->m_alloc_sz == 256);
}
