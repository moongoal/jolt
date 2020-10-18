#include <jolt/test.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/memory/allocator.hpp>

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

    int *c = jolt::memory::reallocate(a, 100, 2000);
    int *d = jolt::memory::reallocate(c, 500, 800);

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

TEST(force_alloc_flags) {
    assert(jolt::memory::get_current_force_flags() == jolt::memory::ALLOC_NONE);

    jolt::memory::force_flags(jolt::memory::ALLOC_BIG | jolt::memory::ALLOC_SCRATCH);
    assert(
      jolt::memory::get_current_force_flags()
      == (jolt::memory::ALLOC_BIG | jolt::memory::ALLOC_SCRATCH));

    jolt::memory::push_force_flags(jolt::memory::ALLOC_PERSIST);
    assert(jolt::memory::get_current_force_flags() == jolt::memory::ALLOC_PERSIST);

    jolt::memory::pop_force_flags();
    assert(
      jolt::memory::get_current_force_flags()
      == (jolt::memory::ALLOC_BIG | jolt::memory::ALLOC_SCRATCH));
}
