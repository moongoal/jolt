#include <test.hpp>
#include <threading/thread.hpp>
#include <memory/allocator.hpp>

void free_mt_handler(void *ptr) { jolt::memory::free(ptr); }

SETUP { jolt::threading::initialize(); }

TEST(allocate__free) {
    size_t before = jolt::memory::get_allocated_size();

    int *a = jolt::memory::allocate<int>(500);
    size_t after = jolt::memory::get_allocated_size();

    jolt::memory::free(a);
    size_t after_del = jolt::memory::get_allocated_size();

    assert2(after > before, "FAIL: Amount of memory should increase with allocation.");
    assert2(after_del < after, "FAIL: Amount of memory should decrease with deletion.");
}

TEST(reallocate) {
    int *a = jolt::memory::allocate<int>(100);
    int *b = jolt::memory::allocate<int>(500);

    int *c = jolt::memory::reallocate(a, 2000);
    int *d = jolt::memory::reallocate(c, 800);

    jolt::memory::free(b);
    jolt::memory::free(d);

    assert(c > a && c > b);
    assert(d == c);
}

TEST(free__mt) {
    size_t const mem_alloc = jolt::memory::get_allocated_size();
    int *a = jolt::memory::allocate<int>(100);
    jolt::threading::Thread t{free_mt_handler};

    t.start(a);
    t.join();

    assert(mem_alloc == jolt::memory::get_allocated_size());
}
