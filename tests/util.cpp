#include <test.hpp>
#include <util.hpp>

using namespace jolt;

TEST(choose) {
    // Unsigned integer value
    assert(choose(10u, 11u, 3 < 5) == 10);
    assert(choose(7u, 13u, 3 != 3) == 13);

    // Signed integer value
    assert(choose(10, 11, 3 < 5) == 10);
    assert(choose(7, 13, 3 != 3) == 13);
    assert(choose(-7, 13, 3 != 3) == 13);
    assert(choose(7, -13, 3 != 3) == -13);
    assert(choose(0, -13, 3 != 3) == -13);
    assert(choose(7, 0, 3 != 3) == 0);
}

TEST(align_raw_ptr) {
    // std::cout << std::hex << align_raw_ptr((void *)1234, 2) << std::endl;

    assert(align_raw_ptr((void *)629, 1) == (void *)629);
    assert(align_raw_ptr((void *)629, 4) == (void *)632);
    assert(align_raw_ptr((void *)629, 8) == (void *)632);
    assert(align_raw_ptr((void *)1234, 2) == (void *)1234);
    assert(align_raw_ptr((void *)1234, 16) == (void *)1248);
    assert(align_raw_ptr((void *)1247, 16) == (void *)1248);
    assert(align_raw_ptr((void *)1248, 16) == (void *)1248);
    assert(align_raw_ptr((void *)1247, 256) == (void *)1280);
}
