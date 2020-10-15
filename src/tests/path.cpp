#include <jolt/test.hpp>
#include <jolt/path.hpp>

using namespace jolt::path;

TEST(normalize) {
    Path p1{"/1/2/3"};
    Path p2{"/1/2/3/"};
    Path p3{"1/2\\3"};
    Path p4{"1/2\\3/"};

    assert2(normalize(p1) == p1, "1");
    assert2(normalize(p2) == p1, "2");
    assert2(normalize(p3) == "1/2/3", "3");
    assert2(normalize(p4) == "1/2/3", "4");
}

TEST(is_absolute) {
    assert2(is_absolute("/1/2"), "Should be absolute");
    assert2(!is_absolute("1/2"), "Should not be absolute");
}
