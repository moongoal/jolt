#include <test.hpp>
#include <threading/thread.hpp>
#include <memory/allocator.hpp>

TEST(allocate__free) {
    jolt::threading::initialize();
    size_t before = jolt::memory::get_allocated_size();

    int *a = jolt::memory::allocate<int>(500);
    size_t after = jolt::memory::get_allocated_size();

    jolt::memory::free(a);
    size_t after_del = jolt::memory::get_allocated_size();

    assert2(after > before, "FAIL: Amount of memory should increase with allocation.");
    assert2(after_del < after, "FAIL: Amount of memory should decrease with deletion.");
}